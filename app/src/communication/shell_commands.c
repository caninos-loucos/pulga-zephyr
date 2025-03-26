#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <communication/uart/uart_interface.h>
#include <sensors/sensors_interface.h>

LOG_MODULE_REGISTER(shell_commands, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// ** Sensors command handlers **

#define HELP_READ_SENSOR "Read sensors and store values in the application buffer."
#define HELP_SAMPLING_INTERVAL "Get or set sensor interface's sampling interval in milliseconds."
#define HELP_SAMPLING_INTERVAL_GET "Get sensor sampling interval. Usage: \"sampling_interval get\"."
#define HELP_SAMPLING_INTERVAL_SET "Set sensor sampling interval. Usage: \"sampling_interval set <INTERVAL>\"."
// Time (`argv[1]`) is in millisenconds
static int set_sampling_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv);
static int get_sampling_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv);
static int read_sensors_cmd_handler(const struct shell *sh, size_t argc, char **argv);

SHELL_CMD_REGISTER(read_sensor, NULL, HELP_READ_SENSOR, read_sensors_cmd_handler);
// Registers get and set as subcommands of sampling_interval
SHELL_STATIC_SUBCMD_SET_CREATE(sampling_interval_subcmds,
                               SHELL_CMD(set, NULL, HELP_SAMPLING_INTERVAL_SET, set_sampling_interval_cmd_handler),
                               SHELL_CMD(get, NULL, HELP_SAMPLING_INTERVAL_GET, get_sampling_interval_cmd_handler),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(sampling_interval, &sampling_interval_subcmds, HELP_SAMPLING_INTERVAL, NULL);

// ** Trasmission command handlers **

#define HELP_FORWARD_DATA "Insert a text item in the application buffer."
#define HELP_TRANSMISSION_INTERVAL "Get or set communication interface's transmission interval in milliseconds."
#define HELP_TRANSMISSION_INTERVAL_GET "Get communication interface's transmission interval. " \
                                       "Usage: \"transmission_interval get\"."
#define HELP_TRANSMISSION_INTERVAL_SET "Set communication interface's transmission interval. " \
                                       "Usage: \"transmission_interval set <INTERVAL>\"."
// Time (`argv[1]`) is in millisenconds
static int set_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv);
static int get_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv);
static int forward_data_cmd_handler(const struct shell *sh, size_t argc, char **argv);

SHELL_CMD_REGISTER(forward_data, NULL, HELP_FORWARD_DATA, forward_data_cmd_handler);
// Registers get and set as subcommands of transmission_interval
SHELL_STATIC_SUBCMD_SET_CREATE(transmission_interval_subcmds,
                               SHELL_CMD(set, NULL, HELP_TRANSMISSION_INTERVAL_SET, set_transmission_interval_cmd_handler),
                               SHELL_CMD(get, NULL, HELP_TRANSMISSION_INTERVAL_GET, get_transmission_interval_cmd_handler),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(transmission_interval, &transmission_interval_subcmds,
                   HELP_TRANSMISSION_INTERVAL, NULL);

/**
 * IMPLEMENTATIONS
 */

// Sensors command handlers

static int set_sampling_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    // Returns if there are not enough arguments
    if (argc != 2)
    {
        shell_error(sh, "Too few arguments.\n%s", HELP_SAMPLING_INTERVAL_SET);
        return -EINVAL;
    }

    int error = 0;
    // Converts string to integer
    int interval = shell_strtol(argv[1], 10, &error);
    if (error != 0)
    {
        shell_error(sh, "Invalid interval.");
        return -EINVAL;
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
    // Returns if there are not enough arguments
    if (argc < 2)
    {
        shell_error(sh, "Too few arguments.\n%s", HELP_READ_SENSOR);
        return -EINVAL;
    }

    // Iterates over the arguments and reads the sensors
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

// Trasmission command handlers

static int set_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{

    // Returns if there are not enough arguments
    if (argc != 2)
    {
        shell_error(sh, "Too few arguments.\n%s", HELP_TRANSMISSION_INTERVAL_SET);
        return -EINVAL;
    }

    int error = 0;
    int interval = shell_strtol(argv[1], 10, &error);
    // Converts string to integer
    if (error != 0)
    {
        shell_error(sh, "Invalid interval");
        return -EINVAL;
    }

    set_transmission_interval(interval);

    return 0;
}

static int get_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Transmission interval is %d milliseconds", get_transmission_interval());

    return 0;
}

static int forward_data_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc != 2)
    {
        shell_error(sh, "Must provide a payload.\n%s", HELP_FORWARD_DATA);
        return -EINVAL;
    }

    char payload[SIZE_32_BIT_WORDS_TO_BYTES((MAX_32_WORDS))] = {0};
    snprintf(payload, sizeof(payload), "%s", argv[1]);

    if (insert_in_buffer(&app_buffer, (uint32_t *)payload, TEXT_DATA, 0, MAX_32_WORDS) != 0)
    {
        shell_error(sh, "Failed to insert data in ring buffer.");
        return -EAGAIN;
    }

    return 0;
}
