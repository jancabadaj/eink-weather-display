#include "updateScheduler.h"
#include "../logger.h"

Planner::Settings UpdateScheduler::settings() const
{
    Planner::Settings settings;
    settings.nightStartHourUtc = _configOverrides.getNightStartHour();
    settings.nightEndHourUtc = _configOverrides.getNightEndHour();
    return settings;
}

Planner::Mode UpdateScheduler::scheduleRefresh(uint64_t dataUtcTimestampMs)
{
    const Planner::Settings config = settings();
    const uint64_t nowUtcMs = _serverClock.getUtcTime();

    logger.info("[UpdateScheduler] Calculating next refresh delay. Current UTC: %llu, Data timestamp: %llu, Age: %llus",
                nowUtcMs, dataUtcTimestampMs, (nowUtcMs - dataUtcTimestampMs) / 1000);

    const Planner::Plan plan = Planner::planNext(nowUtcMs, dataUtcTimestampMs, config);
    const uint64_t now = _clock.uptimeMs();
    const uint64_t delayMs = Planner::throttle(now, plan.delayMs, config.refreshIntervalMs, _rateLimit);

    if (delayMs != plan.delayMs)
    {
        logger.warning("[UpdateScheduler] Rate limit: %d calls within %llus window, throttling next refresh",
                       Config::Schedule::maxCallsPerInterval, config.refreshIntervalMs / 1000);
    }

    _nextRefreshMillis = now + delayMs;

    if (plan.mode == Planner::Mode::Night)
    {
        logger.info("[UpdateScheduler] Night mode - no updates until %d UTC (current: %d UTC, %llus remaining)",
                    config.nightEndHourUtc, Planner::hourOfDayUtc(nowUtcMs), delayMs / 1000);
    }
    else
    {
        logger.info("[UpdateScheduler] Next refresh scheduled in %llu seconds", delayMs / 1000);
    }

    return plan.mode;
}

void UpdateScheduler::scheduleRetry()
{
    _nextRefreshMillis = _clock.uptimeMs() + Config::Schedule::refreshIntervalMs;
    logger.info("[UpdateScheduler] Retry scheduled in %llu seconds",
                Config::Schedule::refreshIntervalMs / 1000);
}
