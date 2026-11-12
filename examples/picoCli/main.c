#include <stdio.h>

#define PICO_IMPLEMENTATION
#include "pico/picoCli.h"

static picoCliFlagInfo_t verboseFlag = {
    .longName  = "verbose",
    .shortName = 'v',
    .help      = "Show more detail"};

static picoCliOptionInfo_t configOption = {
    .longName     = "config",
    .shortName    = 'c',
    .valueName    = "FILE",
    .defaultValue = "pico.toml",
    .help         = "Read settings from FILE"};

static picoCliFlagInfo_t dryRunFlag = {
    .longName  = "dry-run",
    .shortName = 'n',
    .help      = "Print the action without running it"};

static picoCliParamInfo_t runTargetParam = {
    .name  = "TARGET",
    .help  = "Target to run",
    .flags = PICO_CLI_ARGUMENT_REQUIRED};

static picoCliOptionInfo_t projectDirectoryOption = {
    .longName     = "directory",
    .shortName    = 'd',
    .valueName    = "PATH",
    .defaultValue = ".",
    .help         = "Create the project in PATH"};

static picoCliCommandInfo_t runCommand = {
    .name        = "run",
    .summary     = "Run a target",
    .description = "Run the requested target.",
    .pFlags      = &dryRunFlag,
    .flagCount   = 1,
    .pParams     = &runTargetParam,
    .paramCount  = 1};

static picoCliCommandInfo_t projectInitCommand = {
    .name        = "init",
    .summary     = "Create a project",
    .description = "Initialize a new Pico project directory.",
    .pOptions    = &projectDirectoryOption,
    .optionCount = 1};

static picoCliCommandInfo_t projectStatusCommand = {
    .name        = "status",
    .summary     = "Show project status",
    .description = "Display a short status summary for the current project."};

static picoCliCommandInfo_t *const projectCommands[] = {
    &projectInitCommand,
    &projectStatusCommand};

static picoCliCommandInfo_t projectCommand = {
    .name            = "project",
    .summary         = "Manage a project",
    .pSubcommands    = projectCommands,
    .subcommandCount = sizeof(projectCommands) / sizeof(projectCommands[0])};

static picoCliCommandInfo_t *const commands[] = {
    &runCommand,
    &projectCommand};

static picoCliApp_t app = {
    .name         = "pico",
    .version      = "0.1.0",
    .description  = "A small example application built with picoCli.",
    .flags        = PICO_CLI_APP_EXIT_ON_HELP | PICO_CLI_APP_EXIT_ON_VERSION,
    .pFlags       = &verboseFlag,
    .flagCount    = 1,
    .pOptions     = &configOption,
    .optionCount  = 1,
    .pCommands    = commands,
    .commandCount = sizeof(commands) / sizeof(commands[0])};

int main(int argc, char *argv[])
{
    printf("Hello, Pico!\n");
    picoCliStatus status = picoCliParse(&app, argc, argv);

    if (status == PICO_CLI_STATUS_HELP || status == PICO_CLI_STATUS_VERSION) {
        return 0;
    }
    if (status != PICO_CLI_STATUS_OK) {
        return 2;
    }

    printf("Config: %s\n", configOption.value);
    if (verboseFlag.enabled) {
        printf("Verbosity: %zu\n", verboseFlag.occurrenceCount);
    }

    if (app.pCommand == &runCommand) {
        printf("Run target: %s\n", runTargetParam.value);
        printf("Dry run: %s\n", dryRunFlag.enabled ? "yes" : "no");
    } else if (app.pCommand == &projectInitCommand) {
        printf("Initialize project in: %s\n", projectDirectoryOption.value);
    } else if (app.pCommand == &projectStatusCommand) {
        printf("Project status: clean\n");
    } else {
        printf("No command selected. Run 'pico --help' for usage.\n");
    }

    printf("Goodbye, Pico!\n");
    return 0;
}
