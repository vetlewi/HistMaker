//
// Created by Vetle Wegner Ingeberg on 04/05/2020.
//

#include "ScalerTransmitter.h"

#include <InfluxDBFactory.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>

#define LIVETIMEA_ADDRESS 0x0004a37f
#define LIVETIMEB_ADDRESS 0x0004a38f
#define FASTPEAKSA_ADDRESS 0x0004a39f
#define FASTPEAKSB_ADDRESS 0x0004a3af
#define RUNTIMEA_ADDRESS 0x0004a342
#define RUNTIMEB_ADDRESS 0x0004a343
#define CHANEVENTSA_ADDRESS 0x0004a41f
#define CHANEVENTSB_ADDRESS 0x0004a42f

static ScalerTransmitter *private_transmitter = nullptr;

ScalerTransmitter::ScalerTransmitter(const char *url, const int *ts_factor)
    : db( influxdb::InfluxDBFactory::Get(url) )
{

    if ( private_transmitter != nullptr )
        throw std::runtime_error("ScalerTransmitter already set!");
    else
        private_transmitter = this;

    if ( ts_factor != nullptr )
        std::copy(ts_factor, ts_factor+SCALER_LENGTH, timestamp_factor.begin());
}

ScalerTransmitter::~ScalerTransmitter()
{
    private_transmitter = nullptr;
}

void ScalerTransmitter::ProcessScalers(const scaler_t &scalers)
{
    if ( pre_scalers.empty() ) {
        pre_scalers = scalers;
        return;
    } else if ( pre_scalers.size() != scalers.size() ){
        spdlog::error("Length of scalers doesn't match");
        pre_scalers = scalers;
        return;
    }
    // Make sure the buffer is larger than the number of points that will be written
    // reason is that we don't want any exceptions. We want all exceptions to be handled
    // at the end of our processing, when we explicitly tell the influxdb library to
    // flush the buffer.
    db->batchOf((scalers.size() + 2) * 16);
    for ( int module = 0 ; module < scalers.size() ; ++module ){
        for (int channel = 0 ; channel < 16 ; ++channel){

            uint64_t fastPeakN = scalers[module][FASTPEAKSA_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            fastPeakN = fastPeakN << 32;
            fastPeakN += scalers[module][FASTPEAKSB_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t fastPeakP = pre_scalers[module][FASTPEAKSA_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            fastPeakP = fastPeakP << 32;
            fastPeakP += pre_scalers[module][FASTPEAKSB_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double fastPeak = fastPeakN - fastPeakP;

            uint64_t LiveTimeN = scalers[module][LIVETIMEA_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            LiveTimeN = LiveTimeN << 32;
            LiveTimeN |= scalers[module][LIVETIMEB_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t LiveTimeP = pre_scalers[module][LIVETIMEA_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            LiveTimeP = LiveTimeP << 32;
            LiveTimeP |= pre_scalers[module][LIVETIMEB_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double liveTime = LiveTimeN - LiveTimeP;
            if (timestamp_factor[module] == 8)
                liveTime *= 2e-6/250.;
            else
                liveTime *= 1e-6/100.;

            uint64_t ChanEventsN = scalers[module][CHANEVENTSA_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            ChanEventsN = ChanEventsN << 32;
            ChanEventsN |= scalers[module][CHANEVENTSB_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t ChanEventsP = pre_scalers[module][CHANEVENTSA_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            ChanEventsP = ChanEventsP << 32;
            ChanEventsP |= pre_scalers[module][CHANEVENTSB_ADDRESS + channel - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double ChanEvents = ChanEventsN - ChanEventsP;

            uint64_t runTimeN = scalers[module][RUNTIMEA_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            runTimeN = runTimeN << 32;
            runTimeN |= scalers[module][RUNTIMEB_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t runTimeP = pre_scalers[module][RUNTIMEA_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            runTimeP = runTimeP << 32;
            runTimeP |= pre_scalers[module][RUNTIMEB_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double runTime = runTimeN - runTimeP;

            runTime *= 1.0e-6 / 100.;

            //ICR[i][j] = (liveTime !=0) ? fastPeak/liveTime : 0;
            //OCR[i][j] = (runTime != 0) ? ChanEvents/runTime : 0;

            auto time = std::chrono::nanoseconds(runTimeN);

            db->write(influxdb::Point{"count_rate"}
            .addField("ICR", (liveTime !=0) ? fastPeak/liveTime : 0)
            .addField("OCR", (runTime != 0) ? ChanEvents/runTime : 0)
            .addTag("module", std::to_string(module))
            .addTag("channel", std::to_string(channel))
            .setTimestamp(start_time + std::chrono::duration_cast<std::chrono::milliseconds>(time)));
        }
        pre_scalers[module] = scalers[module];
    }

    try {
        db->flushBuffer();
    } catch (const std::exception &e) {
        // Worst case we loose some scalers. However, it doesn't kill our
        // program since it is not critical, just sad :(
        spdlog::error("ScalerTransmitter: " + std::string(e.what()));
        std::cerr << "Error transmitting scalers: " << e.what() << std::endl;
    }


}

void ScalerTransmitter::SetTS_Factor(const int *ts_factor)
{
    if ( ts_factor != nullptr )
        std::copy(ts_factor, ts_factor+SCALER_LENGTH, timestamp_factor.begin());
}

ScalerTransmitter *ScalerTransmitter::Get()
{
    return private_transmitter;
}