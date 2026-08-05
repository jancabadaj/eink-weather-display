#include <doctest.h>

#include "fakes/appFixture.h"

namespace
{
    // A station response with controllable timestamps, so a test can decide
    // whether the data looks new and what hour the device thinks it is.
    std::string stationPayload(uint64_t dataUtcSec, uint64_t serverUtcSec, float pressure = 1013.0f)
    {
        return R"({"body":{"devices":[{"_id":"aa:bb","dashboard_data":{"Temperature":20.5,)"
               R"("Humidity":50,"Pressure":)" +
               std::to_string(pressure) + R"(,"Noise":35,"CO2":600,"time_utc":)" +
               std::to_string(dataUtcSec) + R"(}}]},"time_server":)" +
               std::to_string(serverUtcSec) + "}";
    }

    // 2025-01-15 12:00:00 UTC - comfortably inside the waking window.
    constexpr uint64_t noonUtcSec = 1736942400;
    // 2025-01-15 23:00:00 UTC - inside the default 21:00-05:00 night window.
    constexpr uint64_t nightUtcSec = 1736982000;

    // The pixels a given screen produces, for comparing against what was shown.
    std::vector<uint8_t> nightScreenPixels()
    {
        static FrameBuffer reference;
        Screens screens(reference);
        screens.renderNightModeIndicator();
        return std::vector<uint8_t>(reference.data(), reference.data() + FrameBuffer::byteCount);
    }

    std::vector<uint8_t> errorScreenPixels()
    {
        static FrameBuffer reference;
        Screens screens(reference);
        screens.renderNetworkError();
        return std::vector<uint8_t>(reference.data(), reference.data() + FrameBuffer::byteCount);
    }
} // namespace

TEST_CASE("App: doesNothingUntilTheProviderIsConfigured")
{
    AppFixture f;

    f.app.tick();

    CHECK(f.panel.presentCount == 0);
    CHECK(f.http.calls.empty());
}

TEST_CASE("App: coldStartFetchesAndPresentsOnce")
{
    AppFixture f;
    f.logIn();
    f.http.queueOk(stationPayload(noonUtcSec, noonUtcSec));
    f.http.queueOk(R"({"body":{}})"); // pressure history

    f.app.tick();

    CHECK(f.panel.presentCount == 1);
    CHECK_FALSE(f.app.isUpdateLoopStopped());
}

TEST_CASE("App: doesNotFetchAgainUntilTheScheduleIsDue")
{
    AppFixture f;
    f.logIn();
    f.http.queueOk(stationPayload(noonUtcSec, noonUtcSec));
    f.http.queueOk(R"({"body":{}})");
    f.app.tick();

    const size_t callsAfterFirstFetch = f.http.calls.size();
    f.app.tick();
    f.app.tick();

    CHECK(f.http.calls.size() == callsAfterFirstFetch);
}

TEST_CASE("App: unchangedDataDoesNotRedrawTheDisplay")
{
    AppFixture f;
    f.logIn();
    f.http.queueOk(stationPayload(noonUtcSec, noonUtcSec));
    f.http.queueOk(R"({"body":{}})");
    f.app.tick();
    REQUIRE(f.panel.presentCount == 1);

    // Same data timestamp on the next fetch: nothing new to show.
    f.http.queueOk(stationPayload(noonUtcSec, noonUtcSec + 600));
    f.app.reloadData();

    CHECK(f.panel.presentCount == 1);
}

TEST_CASE("App: newerDataRedrawsTheDisplay")
{
    AppFixture f;
    f.logIn();
    f.http.queueOk(stationPayload(noonUtcSec, noonUtcSec));
    f.http.queueOk(R"({"body":{}})");
    f.app.tick();

    f.http.queueOk(stationPayload(noonUtcSec + 600, noonUtcSec + 600));
    f.app.reloadData();

    CHECK(f.panel.presentCount == 2);
}

TEST_CASE("App: repeatedFailuresShowTheErrorScreenAndStop")
{
    AppFixture f;
    f.logIn();

    for (int attempt = 0; attempt < Config::Schedule::maxConsecutiveFailures; attempt++)
    {
        f.http.queueStatus(500);
        f.app.reloadData();
    }

    CHECK(f.app.isUpdateLoopStopped());
    CHECK(f.panel.presentCount == 1);
    CHECK(f.panel.lastFrame == errorScreenPixels());

    // A stopped loop stays quiet.
    const size_t callsWhenStopped = f.http.calls.size();
    f.app.tick();
    CHECK(f.http.calls.size() == callsWhenStopped);
}

TEST_CASE("App: aFailureShortOfTheLimitOnlySchedulesARetry")
{
    AppFixture f;
    f.logIn();

    f.http.queueStatus(500);
    f.app.reloadData();

    CHECK_FALSE(f.app.isUpdateLoopStopped());
    CHECK(f.panel.presentCount == 0); // nothing redrawn for a single failure
}

TEST_CASE("App: restartResumesAfterAStop")
{
    AppFixture f;
    f.logIn();
    for (int attempt = 0; attempt < Config::Schedule::maxConsecutiveFailures; attempt++)
    {
        f.http.queueStatus(500);
        f.app.reloadData();
    }
    REQUIRE(f.app.isUpdateLoopStopped());

    f.app.restartUpdateLoop();
    f.http.queueOk(stationPayload(noonUtcSec, noonUtcSec));
    f.http.queueOk(R"({"body":{}})");
    f.app.tick();

    CHECK_FALSE(f.app.isUpdateLoopStopped());
    CHECK(f.panel.presentCount == 2); // error screen, then live data
}

TEST_CASE("App: nightTimeShowsTheNightScreenAndPausesUpdates")
{
    AppFixture f;
    f.logIn();
    f.http.queueOk(stationPayload(nightUtcSec, nightUtcSec));
    f.http.queueOk(R"({"body":{}})");

    f.app.tick();

    CHECK(f.panel.presentCount == 1);
    CHECK(f.panel.lastFrame == nightScreenPixels());

    // The next refresh is pushed out to the end of the night, so ticking again
    // does not fetch.
    const size_t callsAfterNight = f.http.calls.size();
    f.app.tick();
    CHECK(f.http.calls.size() == callsAfterNight);
}

TEST_CASE("App: recordsEachReadingInThePressureHistory")
{
    AppFixture f;
    f.logIn();
    f.http.queueOk(stationPayload(noonUtcSec, noonUtcSec, 1010.0f));
    f.http.queueOk(R"({"body":{}})");
    f.app.tick();

    // Far enough ahead to land in a new chart bucket.
    const uint64_t later = noonUtcSec + 3 * 3600;
    f.http.queueOk(stationPayload(later, later, 1004.0f));
    f.app.reloadData();

    CHECK(f.panel.presentCount == 2);
}
