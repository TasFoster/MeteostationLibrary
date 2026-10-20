#include "MeteoService.h"

MeteoService::MeteoService(TempStorage& storage, OutputInterface& output)
    : storage(storage), output(output)
{
}

MeteoService::~MeteoService()
{
}

void MeteoService::display(MeasurementType type)
{
    double value;

    switch (type)
    {
        case MeasurementType::Temperature:
            if (storage.readTemperature(value) == TEMP_STORAGE_SUCCESS)
            {
                String msg = "Temperature: ";
                msg += String(value, METEO_DISPLAY_DECIMAL_PLACES);
                msg += " C";
                output.print(msg);
            }
            break;

        case MeasurementType::Pressure:
            if (storage.readPressure(value) == TEMP_STORAGE_SUCCESS)
            {
                String msg = "Pres: ";
                msg += String(value, METEO_DISPLAY_DECIMAL_PLACES);
                msg += " Pa";
                output.print(msg);
            }
            break;

        case MeasurementType::Humidity:
            if (storage.readHumidity(value) == TEMP_STORAGE_SUCCESS)
            {
                String msg = "Hum: ";
                msg += String(value, METEO_DISPLAY_DECIMAL_PLACES);
                msg += " %";
                output.print(msg);
            }
            break;
    }
}