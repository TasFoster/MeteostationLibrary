#include "MeteoService.h"

MeteoService::MeteoService(TempStorage& storage, OutputInterface& output)
    : storage(storage), output(output)
{
}

MeteoService::~MeteoService()
{
}

void MeteoService::display(char type)
{
    double value;

    switch (type)
    {
        case 't':
            if (storage.readTemperature(value) == 0)
            {
                String msg = "Temperature: ";
                msg += String(value, 2);
                msg += " C";
                output.print(msg);
            }
            break;

        case 'p':
            if (storage.readPressure(value) == 0)
            {
                String msg = "Pres: ";
                msg += String(value, 2);
                msg += " Pa";
                output.print(msg);
            }
            break;

        case 'h':
            if (storage.readHumidity(value) == 0)
            {
                String msg = "Hum: ";
                msg += String(value, 2);
                msg += " %";
                output.print(msg);
            }
            break;
    }
}