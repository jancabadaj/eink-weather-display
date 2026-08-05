#include <doctest.h>

#include "fakes/fixture.h"
#include "fakes/scopedLogCapture.h"
#include "provider/netatmoParse.h"

TEST_CASE("NetatmoParse: readsStationDataFromCapturedResponse")
{
    NetatmoParse::StationData parsed;
    REQUIRE(NetatmoParse::parseStationData(loadFixture("stationData.jsonc"), parsed));

    CHECK(parsed.deviceId == "70:ee:50:12:34:56");

    CHECK(parsed.weather.internal.temperature == doctest::Approx(21.4f));
    CHECK(parsed.weather.internal.humidity == 42);
    CHECK(parsed.weather.internal.pressure == doctest::Approx(1021.9f));
    CHECK(parsed.weather.internal.noise == 37);
    CHECK(parsed.weather.internal.co2 == 903);

    CHECK(parsed.weather.external.temperature == doctest::Approx(-1.5f));
    CHECK(parsed.weather.external.humidity == 84);

    CHECK(parsed.weather.data_timestamp.count() == 1700674814000LL);
    CHECK(parsed.weather.retrieval_timestamp.count() == 1700674855000LL);
}

TEST_CASE("NetatmoParse: rejectsMalformedStationData")
{
    ScopedLogCapture capture;

    NetatmoParse::StationData parsed;
    CHECK_FALSE(NetatmoParse::parseStationData("{not json", parsed));

    // The specific deserialization error is what makes a bad payload diagnosable
    // from the device logs.
    REQUIRE(capture.sink.lines.size() == 1);
    CHECK(capture.sink.lines[0].level == LogLevel::ERROR);
    CHECK(capture.sink.lines[0].message.find("InvalidInput") != std::string::npos);
}

TEST_CASE("NetatmoParse: rejectsStationDataWithoutDevices")
{
    NetatmoParse::StationData parsed;
    CHECK_FALSE(NetatmoParse::parseStationData(R"({"body":{"devices":[]}})", parsed));
}

TEST_CASE("NetatmoParse: missingOutdoorModuleLeavesExternalReadingsZero")
{
    const std::string payload = R"({
        "body": {"devices": [{
            "_id": "aa:bb",
            "dashboard_data": {"Temperature": 20.0, "Humidity": 50, "time_utc": 1700000000}
        }]},
        "time_server": 1700000001
    })";

    NetatmoParse::StationData parsed;
    REQUIRE(NetatmoParse::parseStationData(payload, parsed));

    CHECK(parsed.weather.internal.temperature == doctest::Approx(20.0f));
    CHECK(parsed.weather.external.temperature == doctest::Approx(0.0f));
    CHECK(parsed.weather.external.humidity == 0);
}

TEST_CASE("NetatmoParse: missingFieldsFallBackToZero")
{
    const std::string payload = R"({"body":{"devices":[{"_id":"aa:bb","dashboard_data":{}}]}})";

    NetatmoParse::StationData parsed;
    REQUIRE(NetatmoParse::parseStationData(payload, parsed));

    CHECK(parsed.weather.internal.humidity == 0);
    CHECK(parsed.weather.internal.co2 == 0);
    CHECK(parsed.weather.data_timestamp.count() == 0);
}

TEST_CASE("NetatmoParse: readsTimestampsWiderThanInt32")
{
    // Extracting with an int-typed default makes ArduinoJson fall back to the
    // default for anything past 2038, silently yielding the epoch.
    const std::string payload = R"({
        "body": {"devices": [{"_id": "aa:bb", "dashboard_data": {"time_utc": 2200000000}}]},
        "time_server": 2200000001
    })";

    NetatmoParse::StationData parsed;
    REQUIRE(NetatmoParse::parseStationData(payload, parsed));

    CHECK(parsed.weather.data_timestamp.count() == 2200000000000LL);
    CHECK(parsed.weather.retrieval_timestamp.count() == 2200000001000LL);
}

TEST_CASE("NetatmoParse: readsMeasureFromCapturedResponse")
{
    PressureHistory history;
    REQUIRE(NetatmoParse::parseMeasure(loadFixture("measure.jsonc"), history));

    // The fixture holds far more readings than the chart keeps, so the window
    // ends up full and holding the most recent samples.
    CHECK(history.count == Config::PressureChart::barCount);
    CHECK(history.values[history.count - 1] == doctest::Approx(1015.2f));
    CHECK(history.timestamps[history.count - 1] == 1768228200UL);
}

TEST_CASE("NetatmoParse: skipsMeasureEntriesWithNoValue")
{
    const std::string payload = R"({"body":{"1700000000":[],"1700007200":[1002.5]}})";

    PressureHistory history;
    REQUIRE(NetatmoParse::parseMeasure(payload, history));

    CHECK(history.count == 1);
    CHECK(history.values[0] == doctest::Approx(1002.5f));
}

TEST_CASE("NetatmoParse: rejectsMalformedMeasure")
{
    PressureHistory history;
    CHECK_FALSE(NetatmoParse::parseMeasure("nonsense", history));
}

TEST_CASE("NetatmoParse: readsTokenResponse")
{
    const std::string payload =
        R"({"access_token":"abc123","refresh_token":"def456","expires_in":10800})";

    NetatmoParse::TokenSet tokens;
    REQUIRE(NetatmoParse::parseTokenResponse(payload, tokens));

    CHECK(tokens.accessToken == "abc123");
    CHECK(tokens.refreshToken == "def456");
    CHECK(tokens.expiresInSeconds == 10800);
}

TEST_CASE("NetatmoParse: rejectsTokenResponseMissingRefreshToken")
{
    NetatmoParse::TokenSet tokens;
    CHECK_FALSE(NetatmoParse::parseTokenResponse(R"({"access_token":"abc123"})", tokens));
}

TEST_CASE("NetatmoParse: rejectsMalformedTokenResponse")
{
    NetatmoParse::TokenSet tokens;
    CHECK_FALSE(NetatmoParse::parseTokenResponse("", tokens));
}
