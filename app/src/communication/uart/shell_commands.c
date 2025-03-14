#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <communication/uart/shell_commands.h>
#include <communication/uart/uart_interface.h>
#include <sensors/sensors_interface.h>

LOG_MODULE_REGISTER(shell_commands, CONFIG_APP_LOG_LEVEL);

// Sensors

static int set_sampling_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 1)
        return 0;

    int error = 0;
    int interval = shell_strtol(argv[1], 10, &error);
    if (error != 0)
    {
        shell_print(sh, "Ivalid interval");
        return 0;
    }

    set_sampling_interval(interval);

    return 0;
}

static int get_sampling_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Sampling interval is %d milliseconds", get_sampling_interval());

    return 0;
}

static int read_sensors_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        char *sensor_name = argv[i];
        enum SensorType sensor_num = -1;

        if (!strcmp(sensor_name, "bme280"))
            sensor_num = BME280;
        else if (!strcmp(sensor_name, "bmi160"))
            sensor_num = BMI160;
        else if (!strcmp(sensor_name, "si1133"))
            sensor_num = SI1133;
        else if (!strcmp(sensor_name, "scd30"))
            sensor_num = SCD30;
        else if (!strcmp(sensor_name, "gps"))
            sensor_num = L86_M33;

        if (sensor_num == -1 || sensor_apis[sensor_num] == NULL)
        {
            shell_warn(sh, "Sensor %s is not available", sensor_name);
            continue;
        }

        shell_print(sh, "Reading from %s", sensor_name);
        sensor_apis[sensor_num]->read_sensor_values();
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sampling_interval_subcmds,
                               SHELL_CMD(set, NULL, "Set sensor interval", set_sampling_interval_cmd_handler),
                               SHELL_CMD(get, NULL, "Get sampling interval", get_sampling_interval_cmd_handler),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(sampling_interval, &sampling_interval_subcmds, "Get or set sensor interface's sampling interval", NULL);

SHELL_CMD_REGISTER(read_sensor, NULL, "Read sensors and store values in the buffer", read_sensors_cmd_handler);

// Trasmission

static int set_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{

    if (argc < 1)
        return 0;

    int error = 0;
    int interval = shell_strtol(argv[1], 10, &error);

    if (error != 0)
    {
        shell_print(sh, "Ivalid interval");
        return 0;
    }

    set_transmission_interval(interval);

    return 0;
}

static int get_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Transmission interval is %d milliseconds", get_transmission_interval());

    return 0;
}

static int forward_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc != 2)
    {
        shell_print(sh, "Must provide one payload");
        return 0;
    }

    char payload[SIZE_32_BIT_WORDS_TO_BYTES((MAX_32_WORDS))] = {0};
    uint32_t data_words[MAX_32_WORDS] = {0};

    snprintf(payload, sizeof(payload), "%s", argv[1]);
    memcpy(&data_words, payload, sizeof(payload));

    if (insert_in_buffer(&app_buffer, data_words, TEXT_DATA, 0, MAX_32_WORDS) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
        return -1;
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(transmission_interval_subcmds,
                               SHELL_CMD(set, NULL, "Set transmission interval", set_transmission_interval_cmd_handler),
                               SHELL_CMD(get, NULL, "Get transmission interval", get_transmission_interval_cmd_handler),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(transmission_interval, &transmission_interval_subcmds, "Get or set communication interface's transmission interval", NULL);

SHELL_CMD_REGISTER(forward, NULL, "Insert one or more text items in the buffer", forward_cmd_handler);