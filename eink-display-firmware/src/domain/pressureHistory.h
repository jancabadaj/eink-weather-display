#pragma once

#include "../config.h"

struct PressureHistory
{
    float values[Config::PressureChart::barCount] = {};
    unsigned long timestamps[Config::PressureChart::barCount] = {};
    int count = 0;

    void addReading(unsigned long timestamp, float pressure)
    {
        // Bucket readings by time window so 10-min periodic updates don't fill the array
        constexpr unsigned long bucketSize = (Config::PressureChart::historyHours * 3600UL) / Config::PressureChart::barCount;

        if (count > 0 && timestamp / bucketSize == timestamps[count - 1] / bucketSize)
        {
            values[count - 1] = pressure;
            timestamps[count - 1] = timestamp;
            return;
        }

        if (count >= Config::PressureChart::barCount)
        {
            for (int i = 1; i < count; i++)
            {
                timestamps[i - 1] = timestamps[i];
                values[i - 1] = values[i];
            }
            count--;
        }

        timestamps[count] = timestamp;
        values[count] = pressure;
        count++;
    }

    bool hasGap(unsigned long currentTimestamp, unsigned long maxGapSeconds) const
    {
        if (count == 0)
            return true;
        return (currentTimestamp - timestamps[count - 1]) > maxGapSeconds;
    }

    void clear()
    {
        count = 0;
    }
};
