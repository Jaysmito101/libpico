/*
MIT License

Copyright (c) 2026 Jaysmito Mukherjee (jaysmito101@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef PICO_CLI_H
#define PICO_CLI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PICO_CLI_NO_SHORT_NAME      '\0'
#define PICO_CLI_ERROR_MESSAGE_SIZE 512

typedef enum {
    PICO_CLI_ARGUMENT_NONE     = 0,
    PICO_CLI_ARGUMENT_REQUIRED = 1u << 0
} picoCliArgumentFlags;

typedef enum {
    PICO_CLI_APP_NONE            = 0,
    PICO_CLI_APP_EXIT_ON_HELP    = 1u << 0,
    PICO_CLI_APP_EXIT_ON_VERSION = 1u << 1,
    PICO_CLI_APP_EXIT_ON_ERROR   = 1u << 2
} picoCliAppFlags;

typedef struct {
    const char *longName;
    char shortName;
    const char *help;
    picoCliArgumentFlags flags;
    bool enabled;
    size_t occurrenceCount;
} picoCliFlagInfo_t;

typedef struct {
    const char *longName;
    char shortName;
    const char *valueName;
    const char *defaultValue;
    const char *help;
    picoCliArgumentFlags flags;
    const char *value;
    bool wasProvided;
} picoCliOptionInfo_t;

typedef struct {
    const char *name;
    const char *help;
    const char *defaultValue;
    picoCliArgumentFlags flags;
    const char *value;
    bool wasProvided;
} picoCliParamInfo_t;

typedef struct picoCliCommandInfo_t picoCliCommandInfo_t;

struct picoCliCommandInfo_t {
    const char *name;
    const char *summary;
    const char *description;
    picoCliFlagInfo_t *pFlags;
    size_t flagCount;
    picoCliOptionInfo_t *pOptions;
    size_t optionCount;
    picoCliParamInfo_t *pParams;
    size_t paramCount;
    picoCliCommandInfo_t *const *pSubcommands;
    size_t subcommandCount;
};

typedef struct picoCliApp_t {
    const char *name;
    const char *version;
    const char *description;
    uint32_t flags;
    picoCliFlagInfo_t *pFlags;
    size_t flagCount;
    picoCliOptionInfo_t *pOptions;
    size_t optionCount;
    picoCliCommandInfo_t *const *pCommands;
    size_t commandCount;

    const picoCliCommandInfo_t *pCommand;
    const picoCliCommandInfo_t *pHelpCommand;
    char error[PICO_CLI_ERROR_MESSAGE_SIZE];
} picoCliApp_t;

typedef enum {
    PICO_CLI_STATUS_OK = 0,
    PICO_CLI_STATUS_HELP,
    PICO_CLI_STATUS_VERSION,
    PICO_CLI_STATUS_ERROR
} picoCliStatus;

/*
 * Parse the command line arguments according to the provided app descriptor.
 *
 * Parsing does not allocate or copy values. Each descriptor retains its typed
 * parsed state. Schema arrays must be writable because parsing resets and
 * updates their state. String values point into argv or to the descriptor's default.
 * App options are global and can appear before or after a command.
 */
picoCliStatus picoCliParse(picoCliApp_t *app, int argc, char *const argv[]);

#ifdef PICO_IMPLEMENTATION

#ifndef PICO_CLI_IMPLEMENTATION
#define PICO_CLI_IMPLEMENTATION

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define PRIV__PICO_CLI_MAX_COMMAND_DEPTH 64

typedef struct {
    picoCliApp_t *pApp;
    int argc;
    char *const *argv;
    int index;
    bool optionsEnded;
} PRIV__picoCliParseContext_t;

typedef struct {
    picoCliFlagInfo_t *pFlag;
    picoCliOptionInfo_t *pOption;
} PRIV__picoCliArgument_t;

static void PRIV__picoCliSetError(picoCliApp_t *app, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(app->error, sizeof(app->error), format, args);
    va_end(args);
}

static void PRIV__picoCliResetFlags(picoCliFlagInfo_t *flags, size_t count)
{
    size_t i;
    if (flags == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        flags[i].enabled         = false;
        flags[i].occurrenceCount = 0;
    }
}

static void PRIV__picoCliResetOptions(picoCliOptionInfo_t *options, size_t count)
{
    size_t i;
    if (options == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        options[i].value       = options[i].defaultValue;
        options[i].wasProvided = false;
    }
}

static void PRIV__picoCliResetParams(picoCliParamInfo_t *params, size_t count)
{
    size_t i;
    if (params == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        params[i].value       = params[i].defaultValue;
        params[i].wasProvided = false;
    }
}

static void PRIV__picoCliResetCommandList(picoCliCommandInfo_t *const *commands, size_t count, size_t depth)
{
    size_t i;
    if (commands == NULL || depth > PRIV__PICO_CLI_MAX_COMMAND_DEPTH) {
        return;
    }
    for (i = 0; i < count; i++) {
        picoCliCommandInfo_t *command = commands[i];
        if (command == NULL) {
            continue;
        }
        PRIV__picoCliResetFlags(command->pFlags, command->flagCount);
        PRIV__picoCliResetOptions(command->pOptions, command->optionCount);
        PRIV__picoCliResetParams(command->pParams, command->paramCount);
        PRIV__picoCliResetCommandList(command->pSubcommands, command->subcommandCount, depth + 1);
    }
}

static void PRIV__picoCliResetApp(picoCliApp_t *app)
{
    app->pCommand     = NULL;
    app->pHelpCommand = NULL;
    app->error[0]     = '\0';
    PRIV__picoCliResetFlags(app->pFlags, app->flagCount);
    PRIV__picoCliResetOptions(app->pOptions, app->optionCount);
    PRIV__picoCliResetCommandList(app->pCommands, app->commandCount, 0);
}

static bool PRIV__picoCliValidName(const char *name)
{
    return name != NULL && name[0] != '\0';
}

static bool PRIV__picoCliValidateFlags(picoCliApp_t *app, const picoCliFlagInfo_t *flags, size_t count)
{
    size_t i;
    size_t j;
    if (count > 0 && flags == NULL) {
        PRIV__picoCliSetError(app, "flag count is non-zero but the flag array is null");
        return false;
    }
    for (i = 0; i < count; i++) {
        if (!PRIV__picoCliValidName(flags[i].longName)) {
            PRIV__picoCliSetError(app, "each flag needs a long name");
            return false;
        }
        if (strcmp(flags[i].longName, "help") == 0 || strcmp(flags[i].longName, "version") == 0 || flags[i].shortName == 'h') {
            PRIV__picoCliSetError(app, "flag '--%s' conflicts with a built-in option", flags[i].longName);
            return false;
        }
        for (j = 0; j < i; j++) {
            if (strcmp(flags[i].longName, flags[j].longName) == 0 ||
                (flags[i].shortName != PICO_CLI_NO_SHORT_NAME && flags[i].shortName == flags[j].shortName)) {
                PRIV__picoCliSetError(app, "duplicate flag name or short name near '--%s'", flags[i].longName);
                return false;
            }
        }
    }
    return true;
}

static bool PRIV__picoCliValidateOptions(picoCliApp_t *app, const picoCliOptionInfo_t *options, size_t count)
{
    size_t i;
    size_t j;
    if (count > 0 && options == NULL) {
        PRIV__picoCliSetError(app, "option count is non-zero but the option array is null");
        return false;
    }
    for (i = 0; i < count; i++) {
        if (!PRIV__picoCliValidName(options[i].longName)) {
            PRIV__picoCliSetError(app, "each value option needs a long name");
            return false;
        }
        if (strcmp(options[i].longName, "help") == 0 || strcmp(options[i].longName, "version") == 0 || options[i].shortName == 'h') {
            PRIV__picoCliSetError(app, "option '--%s' conflicts with a built-in option", options[i].longName);
            return false;
        }
        for (j = 0; j < i; j++) {
            if (strcmp(options[i].longName, options[j].longName) == 0 ||
                (options[i].shortName != PICO_CLI_NO_SHORT_NAME && options[i].shortName == options[j].shortName)) {
                PRIV__picoCliSetError(app, "duplicate option name or short name near '--%s'", options[i].longName);
                return false;
            }
        }
    }
    return true;
}

static bool PRIV__picoCliValidateArgumentNames(picoCliApp_t *app, const picoCliFlagInfo_t *flags, size_t flagCount,
                                               const picoCliOptionInfo_t *options, size_t optionCount)
{
    size_t i;
    size_t j;
    for (i = 0; i < flagCount; i++) {
        for (j = 0; j < optionCount; j++) {
            if (strcmp(flags[i].longName, options[j].longName) == 0 ||
                (flags[i].shortName != PICO_CLI_NO_SHORT_NAME && flags[i].shortName == options[j].shortName)) {
                PRIV__picoCliSetError(app, "flag '--%s' conflicts with a value option", flags[i].longName);
                return false;
            }
        }
    }
    return true;
}

static bool PRIV__picoCliValidateParams(picoCliApp_t *app, const picoCliParamInfo_t *params, size_t count)
{
    size_t i;
    if (count > 0 && params == NULL) {
        PRIV__picoCliSetError(app, "parameter count is non-zero but the parameter array is null");
        return false;
    }
    for (i = 0; i < count; i++) {
        if (!PRIV__picoCliValidName(params[i].name)) {
            PRIV__picoCliSetError(app, "each positional parameter needs a name");
            return false;
        }
    }
    return true;
}

static bool PRIV__picoCliValidateCommandList(picoCliApp_t *app, picoCliCommandInfo_t *const *commands, size_t count, size_t depth)
{
    size_t i;
    size_t j;
    if (depth > PRIV__PICO_CLI_MAX_COMMAND_DEPTH) {
        PRIV__picoCliSetError(app, "command nesting exceeds the maximum depth");
        return false;
    }
    if (count > 0 && commands == NULL) {
        PRIV__picoCliSetError(app, "command count is non-zero but the command array is null");
        return false;
    }
    for (i = 0; i < count; i++) {
        picoCliCommandInfo_t *command = commands[i];
        if (command == NULL) {
            PRIV__picoCliSetError(app, "command list contains a null command");
            return false;
        }
        if (!PRIV__picoCliValidName(command->name) || strcmp(command->name, "help") == 0) {
            PRIV__picoCliSetError(app, "each command needs a name; 'help' is reserved");
            return false;
        }
        for (j = 0; j < i; j++) {
            if (strcmp(command->name, commands[j]->name) == 0) {
                PRIV__picoCliSetError(app, "duplicate command name '%s'", command->name);
                return false;
            }
        }
        if (!PRIV__picoCliValidateFlags(app, command->pFlags, command->flagCount) ||
            !PRIV__picoCliValidateOptions(app, command->pOptions, command->optionCount) ||
            !PRIV__picoCliValidateArgumentNames(app, command->pFlags, command->flagCount,
                                                command->pOptions, command->optionCount) ||
            !PRIV__picoCliValidateParams(app, command->pParams, command->paramCount) ||
            !PRIV__picoCliValidateCommandList(app, command->pSubcommands, command->subcommandCount, depth + 1)) {
            return false;
        }
    }
    return true;
}

static bool PRIV__picoCliValidateAppCommandNames(picoCliApp_t *app, picoCliCommandInfo_t *const *commands, size_t count)
{
    size_t i;
    size_t j;
    size_t k;
    for (i = 0; i < count; i++) {
        picoCliCommandInfo_t *command = commands[i];
        for (j = 0; j < app->flagCount; j++) {
            for (k = 0; k < command->flagCount; k++) {
                if (strcmp(app->pFlags[j].longName, command->pFlags[k].longName) == 0 ||
                    (app->pFlags[j].shortName != PICO_CLI_NO_SHORT_NAME &&
                     app->pFlags[j].shortName == command->pFlags[k].shortName)) {
                    PRIV__picoCliSetError(app, "app flag '--%s' conflicts with a flag on command '%s'",
                                          app->pFlags[j].longName, command->name);
                    return false;
                }
            }
            for (k = 0; k < command->optionCount; k++) {
                if (strcmp(app->pFlags[j].longName, command->pOptions[k].longName) == 0 ||
                    (app->pFlags[j].shortName != PICO_CLI_NO_SHORT_NAME &&
                     app->pFlags[j].shortName == command->pOptions[k].shortName)) {
                    PRIV__picoCliSetError(app, "app flag '--%s' conflicts with an option on command '%s'",
                                          app->pFlags[j].longName, command->name);
                    return false;
                }
            }
        }
        for (j = 0; j < app->optionCount; j++) {
            for (k = 0; k < command->flagCount; k++) {
                if (strcmp(app->pOptions[j].longName, command->pFlags[k].longName) == 0 ||
                    (app->pOptions[j].shortName != PICO_CLI_NO_SHORT_NAME &&
                     app->pOptions[j].shortName == command->pFlags[k].shortName)) {
                    PRIV__picoCliSetError(app, "app option '--%s' conflicts with a flag on command '%s'",
                                          app->pOptions[j].longName, command->name);
                    return false;
                }
            }
            for (k = 0; k < command->optionCount; k++) {
                if (strcmp(app->pOptions[j].longName, command->pOptions[k].longName) == 0 ||
                    (app->pOptions[j].shortName != PICO_CLI_NO_SHORT_NAME &&
                     app->pOptions[j].shortName == command->pOptions[k].shortName)) {
                    PRIV__picoCliSetError(app, "app option '--%s' conflicts with an option on command '%s'",
                                          app->pOptions[j].longName, command->name);
                    return false;
                }
            }
        }
        if (!PRIV__picoCliValidateAppCommandNames(app, command->pSubcommands, command->subcommandCount)) {
            return false;
        }
    }
    return true;
}

static picoCliFlagInfo_t *PRIV__picoCliFindLongFlag(picoCliFlagInfo_t *flags, size_t count,
                                                    const char *name, size_t nameLength)
{
    size_t i;
    for (i = 0; i < count; i++) {
        if (strlen(flags[i].longName) == nameLength && strncmp(flags[i].longName, name, nameLength) == 0) {
            return &flags[i];
        }
    }
    return NULL;
}

static picoCliOptionInfo_t *PRIV__picoCliFindLongOption(picoCliOptionInfo_t *options, size_t count,
                                                        const char *name, size_t nameLength)
{
    size_t i;
    for (i = 0; i < count; i++) {
        if (strlen(options[i].longName) == nameLength && strncmp(options[i].longName, name, nameLength) == 0) {
            return &options[i];
        }
    }
    return NULL;
}

static picoCliFlagInfo_t *PRIV__picoCliFindShortFlag(picoCliFlagInfo_t *flags, size_t count, char shortName)
{
    size_t i;
    for (i = 0; i < count; i++) {
        if (flags[i].shortName == shortName) {
            return &flags[i];
        }
    }
    return NULL;
}

static picoCliOptionInfo_t *PRIV__picoCliFindShortOption(picoCliOptionInfo_t *options, size_t count, char shortName)
{
    size_t i;
    for (i = 0; i < count; i++) {
        if (options[i].shortName == shortName) {
            return &options[i];
        }
    }
    return NULL;
}

static PRIV__picoCliArgument_t PRIV__picoCliFindArgument(picoCliApp_t *app, picoCliCommandInfo_t *command,
                                                         const char *name, size_t nameLength, char shortName, bool byShortName)
{
    PRIV__picoCliArgument_t argument = {NULL, NULL};
    if (command != NULL) {
        argument.pFlag = byShortName ? PRIV__picoCliFindShortFlag(command->pFlags, command->flagCount, shortName)
                                     : PRIV__picoCliFindLongFlag(command->pFlags, command->flagCount, name, nameLength);
        if (argument.pFlag != NULL) {
            return argument;
        }
        argument.pOption = byShortName ? PRIV__picoCliFindShortOption(command->pOptions, command->optionCount, shortName)
                                       : PRIV__picoCliFindLongOption(command->pOptions, command->optionCount, name, nameLength);
        if (argument.pOption != NULL) {
            return argument;
        }
    }
    argument.pFlag = byShortName ? PRIV__picoCliFindShortFlag(app->pFlags, app->flagCount, shortName)
                                 : PRIV__picoCliFindLongFlag(app->pFlags, app->flagCount, name, nameLength);
    if (argument.pFlag != NULL) {
        return argument;
    }
    argument.pOption = byShortName ? PRIV__picoCliFindShortOption(app->pOptions, app->optionCount, shortName)
                                   : PRIV__picoCliFindLongOption(app->pOptions, app->optionCount, name, nameLength);
    return argument;
}

static picoCliCommandInfo_t *PRIV__picoCliFindCommand(picoCliApp_t *app,
                                                      picoCliCommandInfo_t *parent, const char *name)
{
    picoCliCommandInfo_t *const *commands = parent == NULL ? app->pCommands : parent->pSubcommands;
    size_t count                          = parent == NULL ? app->commandCount : parent->subcommandCount;
    size_t i;
    if (name == NULL) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        if (commands[i] != NULL && strcmp(commands[i]->name, name) == 0) {
            return commands[i];
        }
    }
    return NULL;
}

static picoCliStatus PRIV__picoCliParseError(PRIV__picoCliParseContext_t *context, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(context->pApp->error, sizeof(context->pApp->error), format, args);
    va_end(args);
    return PICO_CLI_STATUS_ERROR;
}

static picoCliStatus PRIV__picoCliConsumeArgument(PRIV__picoCliParseContext_t *context, PRIV__picoCliArgument_t argument,
                                                  const char *inlineValue, bool hasInlineValue, const char *spelling)
{
    const char *value = inlineValue;
    if (argument.pFlag != NULL) {
        if (hasInlineValue) {
            return PRIV__picoCliParseError(context, "flag '%s' does not take a value", spelling);
        }
        argument.pFlag->enabled = true;
        argument.pFlag->occurrenceCount++;
        return PICO_CLI_STATUS_OK;
    }

    if (argument.pOption->wasProvided) {
        return PRIV__picoCliParseError(context, "option '%s' may only be specified once", spelling);
    }
    if (!hasInlineValue) {
        if (context->index + 1 >= context->argc) {
            return PRIV__picoCliParseError(context, "option '%s' requires a value", spelling);
        }
        context->index++;
        value = context->argv[context->index];
    }
    if (value == NULL) {
        return PRIV__picoCliParseError(context, "value for option '%s' is null", spelling);
    }
    argument.pOption->value       = value;
    argument.pOption->wasProvided = true;
    return PICO_CLI_STATUS_OK;
}

static picoCliStatus PRIV__picoCliFinishCommand(PRIV__picoCliParseContext_t *context, const picoCliCommandInfo_t *command)
{
    size_t i;
    if (command == NULL) {
        return PICO_CLI_STATUS_OK;
    }
    for (i = 0; i < command->flagCount; i++) {
        if ((command->pFlags[i].flags & PICO_CLI_ARGUMENT_REQUIRED) != 0 && !command->pFlags[i].enabled) {
            return PRIV__picoCliParseError(context, "required flag '--%s' is missing", command->pFlags[i].longName);
        }
    }
    for (i = 0; i < command->optionCount; i++) {
        if ((command->pOptions[i].flags & PICO_CLI_ARGUMENT_REQUIRED) != 0 &&
            !command->pOptions[i].wasProvided && command->pOptions[i].defaultValue == NULL) {
            return PRIV__picoCliParseError(context, "required option '--%s' is missing", command->pOptions[i].longName);
        }
    }
    for (i = 0; i < command->paramCount; i++) {
        if ((command->pParams[i].flags & PICO_CLI_ARGUMENT_REQUIRED) != 0 &&
            !command->pParams[i].wasProvided && command->pParams[i].defaultValue == NULL) {
            return PRIV__picoCliParseError(context, "required parameter <%s> is missing", command->pParams[i].name);
        }
    }
    return PICO_CLI_STATUS_OK;
}

static picoCliStatus PRIV__picoCliFinishApp(PRIV__picoCliParseContext_t *context)
{
    size_t i;
    for (i = 0; i < context->pApp->flagCount; i++) {
        if ((context->pApp->pFlags[i].flags & PICO_CLI_ARGUMENT_REQUIRED) != 0 && !context->pApp->pFlags[i].enabled) {
            return PRIV__picoCliParseError(context, "required flag '--%s' is missing", context->pApp->pFlags[i].longName);
        }
    }
    for (i = 0; i < context->pApp->optionCount; i++) {
        if ((context->pApp->pOptions[i].flags & PICO_CLI_ARGUMENT_REQUIRED) != 0 &&
            !context->pApp->pOptions[i].wasProvided && context->pApp->pOptions[i].defaultValue == NULL) {
            return PRIV__picoCliParseError(context, "required option '--%s' is missing", context->pApp->pOptions[i].longName);
        }
    }
    return PICO_CLI_STATUS_OK;
}

static picoCliStatus PRIV__picoCliAssignParam(PRIV__picoCliParseContext_t *context, picoCliCommandInfo_t *command,
                                              size_t *paramIndex, const char *value)
{
    while (*paramIndex < command->paramCount) {
        picoCliParamInfo_t *param = &command->pParams[*paramIndex];
        if (!param->wasProvided) {
            param->value       = value;
            param->wasProvided = true;
            (*paramIndex)++;
            return PICO_CLI_STATUS_OK;
        }
        (*paramIndex)++;
    }
    return PRIV__picoCliParseError(context, "unexpected argument '%s'", value);
}

static picoCliCommandInfo_t *PRIV__picoCliResolveHelpPath(PRIV__picoCliParseContext_t *context,
                                                          picoCliCommandInfo_t *command)
{
    picoCliCommandInfo_t *target = command;
    while (context->index < context->argc) {
        const char *name = context->argv[context->index];
        picoCliCommandInfo_t *next;
        if (name == NULL) {
            break;
        }
        next = PRIV__picoCliFindCommand(context->pApp, target, name);
        if (next == NULL) {
            break;
        }
        target = next;
        context->index++;
    }
    return target;
}

static picoCliStatus PRIV__picoCliSetHelpStatus(PRIV__picoCliParseContext_t *context, picoCliCommandInfo_t *command)
{
    context->pApp->pHelpCommand = PRIV__picoCliResolveHelpPath(context, command);
    return PICO_CLI_STATUS_HELP;
}

static picoCliStatus PRIV__picoCliParseScope(PRIV__picoCliParseContext_t *context, picoCliCommandInfo_t *command)
{
    size_t paramIndex = 0;
    while (context->index < context->argc) {
        const char *token = context->argv[context->index];

        if (token == NULL) {
            return PRIV__picoCliParseError(context, "argument at index %d is null", context->index);
        }
        if (!context->optionsEnded && strcmp(token, "help") == 0) {
            context->index++;
            return PRIV__picoCliSetHelpStatus(context, command);
        }
        if (!context->optionsEnded && strcmp(token, "--help") == 0) {
            context->index++;
            return PRIV__picoCliSetHelpStatus(context, command);
        }
        if (!context->optionsEnded && strcmp(token, "--version") == 0 && context->pApp->version != NULL) {
            return PICO_CLI_STATUS_VERSION;
        }
        if (!context->optionsEnded && strcmp(token, "--") == 0) {
            context->optionsEnded = true;
            context->index++;
            continue;
        }

        if (!context->optionsEnded && token[0] == '-' && token[1] == '-') {
            const char *name                 = token + 2;
            const char *equals               = strchr(name, '=');
            size_t nameLength                = equals == NULL ? strlen(name) : (size_t)(equals - name);
            PRIV__picoCliArgument_t argument = PRIV__picoCliFindArgument(context->pApp, command, name, nameLength,
                                                                         PICO_CLI_NO_SHORT_NAME, false);
            picoCliStatus status;
            if (argument.pFlag == NULL && argument.pOption == NULL) {
                return PRIV__picoCliParseError(context, "unknown option '%s'", token);
            }
            status = PRIV__picoCliConsumeArgument(context, argument, equals == NULL ? NULL : equals + 1,
                                                  equals != NULL, token);
            if (status != PICO_CLI_STATUS_OK) {
                return status;
            }
            context->index++;
            continue;
        }

        if (!context->optionsEnded && token[0] == '-' && token[1] != '\0') {
            const char *shortOptions = token + 1;
            size_t shortIndex        = 0;
            while (shortOptions[shortIndex] != '\0') {
                char shortName = shortOptions[shortIndex++];
                PRIV__picoCliArgument_t argument;
                picoCliStatus status;
                if (shortName == 'h') {
                    context->index++;
                    return PRIV__picoCliSetHelpStatus(context, command);
                }
                argument = PRIV__picoCliFindArgument(context->pApp, command, NULL, 0, shortName, true);
                if (argument.pFlag == NULL && argument.pOption == NULL) {
                    return PRIV__picoCliParseError(context, "unknown option '-%c'", shortName);
                }
                if (argument.pFlag != NULL) {
                    status = PRIV__picoCliConsumeArgument(context, argument, NULL, false, token);
                    if (status != PICO_CLI_STATUS_OK) {
                        return status;
                    }
                    continue;
                }
                {
                    const char *inlineValue = shortOptions + shortIndex;
                    bool hasInlineValue     = inlineValue[0] != '\0';
                    if (hasInlineValue && inlineValue[0] == '=') {
                        inlineValue++;
                    }
                    status = PRIV__picoCliConsumeArgument(context, argument, inlineValue, hasInlineValue, token);
                    if (status != PICO_CLI_STATUS_OK) {
                        return status;
                    }
                }
                break;
            }
            context->index++;
            continue;
        }

        if (!context->optionsEnded) {
            picoCliCommandInfo_t *subcommand = PRIV__picoCliFindCommand(context->pApp, command, token);
            if (subcommand != NULL) {
                picoCliStatus status = PRIV__picoCliFinishCommand(context, command);
                if (status != PICO_CLI_STATUS_OK) {
                    return status;
                }
                context->pApp->pCommand = subcommand;
                context->index++;
                return PRIV__picoCliParseScope(context, subcommand);
            }
        }
        if (command == NULL) {
            return PRIV__picoCliParseError(context, "unknown command or unexpected argument '%s'", token);
        }
        {
            picoCliStatus status = PRIV__picoCliAssignParam(context, command, &paramIndex, token);
            if (status != PICO_CLI_STATUS_OK) {
                return status;
            }
        }
        context->index++;
    }
    return PRIV__picoCliFinishCommand(context, command);
}

static bool PRIV__picoCliFindCommandPathRecursive(picoCliCommandInfo_t *const *commands, size_t count,
                                                  const picoCliCommandInfo_t *target,
                                                  const picoCliCommandInfo_t **path, size_t depth, size_t *pathLength)
{
    size_t i;
    if (depth >= PRIV__PICO_CLI_MAX_COMMAND_DEPTH) {
        return false;
    }
    for (i = 0; i < count; i++) {
        const picoCliCommandInfo_t *command = commands[i];
        path[depth]                         = command;
        if (command == target) {
            *pathLength = depth + 1;
            return true;
        }
        if (PRIV__picoCliFindCommandPathRecursive(command->pSubcommands, command->subcommandCount,
                                                  target, path, depth + 1, pathLength)) {
            return true;
        }
    }
    return false;
}

static void PRIV__picoCliPrintFlag(FILE *stream, const picoCliFlagInfo_t *flag)
{
    char label[256];
    if (flag->shortName != PICO_CLI_NO_SHORT_NAME) {
        snprintf(label, sizeof(label), "-%c, --%s", flag->shortName, flag->longName);
    } else {
        snprintf(label, sizeof(label), "    --%s", flag->longName);
    }
    fprintf(stream, "  %-30s %s\n", label, flag->help == NULL ? "" : flag->help);
}

static void PRIV__picoCliPrintOption(FILE *stream, const picoCliOptionInfo_t *option)
{
    char label[256];
    const char *valueName = option->valueName == NULL ? "VALUE" : option->valueName;
    if (option->shortName != PICO_CLI_NO_SHORT_NAME) {
        snprintf(label, sizeof(label), "-%c, --%s <%s>", option->shortName, option->longName, valueName);
    } else {
        snprintf(label, sizeof(label), "    --%s <%s>", option->longName, valueName);
    }
    fprintf(stream, "  %-30s %s", label, option->help == NULL ? "" : option->help);
    if (option->defaultValue != NULL) {
        fprintf(stream, " (default: %s)", option->defaultValue);
    }
    fputc('\n', stream);
}

static void PRIV__picoCliPrintOptionSection(const picoCliApp_t *app, const picoCliCommandInfo_t *command, FILE *stream)
{
    size_t i;
    fprintf(stream, "Options:\n");
    for (i = 0; i < app->flagCount; i++) {
        PRIV__picoCliPrintFlag(stream, &app->pFlags[i]);
    }
    for (i = 0; i < app->optionCount; i++) {
        PRIV__picoCliPrintOption(stream, &app->pOptions[i]);
    }
    if (command != NULL) {
        for (i = 0; i < command->flagCount; i++) {
            PRIV__picoCliPrintFlag(stream, &command->pFlags[i]);
        }
        for (i = 0; i < command->optionCount; i++) {
            PRIV__picoCliPrintOption(stream, &command->pOptions[i]);
        }
    }
    fprintf(stream, "  %-30s %s\n", "-h, --help", "Show this help message");
    if (app->version != NULL) {
        fprintf(stream, "  %-30s %s\n", "    --version", "Show version information");
    }
}

static void PRIV__picoCliPrintHelpInternal(const picoCliApp_t *app, const picoCliCommandInfo_t *command, FILE *stream)
{
    const picoCliCommandInfo_t *path[PRIV__PICO_CLI_MAX_COMMAND_DEPTH];
    size_t pathLength = 0;
    size_t i;
    if (app == NULL || app->name == NULL || stream == NULL) {
        return;
    }
    if (command != NULL && !PRIV__picoCliFindCommandPathRecursive(app->pCommands, app->commandCount, command, path, 0, &pathLength)) {
        command    = NULL;
        pathLength = 0;
    }
    fprintf(stream, "Usage: %s", app->name);
    for (i = 0; i < pathLength; i++) {
        fprintf(stream, " %s", path[i]->name);
    }
    fprintf(stream, " [OPTIONS]");
    if (command != NULL) {
        for (i = 0; i < command->paramCount; i++) {
            const picoCliParamInfo_t *param = &command->pParams[i];
            fprintf(stream, (param->flags & PICO_CLI_ARGUMENT_REQUIRED) != 0 ? " <%s>" : " [%s]", param->name);
        }
        if (command->subcommandCount > 0) {
            fprintf(stream, " [COMMAND]");
        }
    } else if (app->commandCount > 0) {
        fprintf(stream, " [COMMAND]");
    }
    fputc('\n', stream);

    if (command == NULL && app->description != NULL && app->description[0] != '\0') {
        fprintf(stream, "\n%s\n", app->description);
    } else if (command != NULL && command->description != NULL && command->description[0] != '\0') {
        fprintf(stream, "\n%s\n", command->description);
    } else if (command != NULL && command->summary != NULL && command->summary[0] != '\0') {
        fprintf(stream, "\n%s\n", command->summary);
    }

    fputc('\n', stream);
    PRIV__picoCliPrintOptionSection(app, command, stream);

    if (command != NULL && command->paramCount > 0) {
        fprintf(stream, "\nArguments:\n");
        for (i = 0; i < command->paramCount; i++) {
            const picoCliParamInfo_t *param = &command->pParams[i];
            fprintf(stream, "  %-30s %s", param->name, param->help == NULL ? "" : param->help);
            if (param->defaultValue != NULL) {
                fprintf(stream, " (default: %s)", param->defaultValue);
            }
            fputc('\n', stream);
        }
    }

    if ((command == NULL && app->commandCount > 0) || (command != NULL && command->subcommandCount > 0)) {
        picoCliCommandInfo_t *const *commands = command == NULL ? app->pCommands : command->pSubcommands;
        size_t count                          = command == NULL ? app->commandCount : command->subcommandCount;
        fprintf(stream, "\nCommands:\n");
        for (i = 0; i < count; i++) {
            fprintf(stream, "  %-30s %s\n", commands[i]->name, commands[i]->summary == NULL ? "" : commands[i]->summary);
        }
        fprintf(stream, "  %-30s %s\n", "help", "Show help for a command");
    }
}

static void PRIV__picoCliPrintVersionInternal(const picoCliApp_t *app)
{
    fprintf(stdout, "%s%s%s\n", app->name == NULL ? "" : app->name,
            app->version == NULL ? "" : " ", app->version == NULL ? "" : app->version);
}

static picoCliStatus PRIV__picoCliHandleStatus(picoCliApp_t *app, picoCliStatus status)
{
    if (status == PICO_CLI_STATUS_HELP) {
        PRIV__picoCliPrintHelpInternal(app, app->pHelpCommand, stdout);
        if ((app->flags & PICO_CLI_APP_EXIT_ON_HELP) != 0) {
            exit(EXIT_SUCCESS);
        }
    } else if (status == PICO_CLI_STATUS_VERSION) {
        PRIV__picoCliPrintVersionInternal(app);
        if ((app->flags & PICO_CLI_APP_EXIT_ON_VERSION) != 0) {
            exit(EXIT_SUCCESS);
        }
    } else if (status == PICO_CLI_STATUS_ERROR) {
        fprintf(stderr, "%s: error: %s\n", app->name == NULL ? "picoCli" : app->name, app->error);
        if ((app->flags & PICO_CLI_APP_EXIT_ON_ERROR) != 0) {
            exit(EXIT_FAILURE);
        }
    }
    return status;
}

picoCliStatus picoCliParse(picoCliApp_t *app, int argc, char *const argv[])
{
    PRIV__picoCliParseContext_t context;
    picoCliStatus status;
    if (app == NULL) {
        fprintf(stderr, "picoCli: error: app is null\n");
        return PICO_CLI_STATUS_ERROR;
    }

    PRIV__picoCliResetApp(app);
    if (!PRIV__picoCliValidName(app->name) || argc < 0 || (argc > 0 && argv == NULL)) {
        PRIV__picoCliSetError(app, "invalid app definition or argv");
        return PRIV__picoCliHandleStatus(app, PICO_CLI_STATUS_ERROR);
    }
    if (!PRIV__picoCliValidateFlags(app, app->pFlags, app->flagCount) ||
        !PRIV__picoCliValidateOptions(app, app->pOptions, app->optionCount) ||
        !PRIV__picoCliValidateArgumentNames(app, app->pFlags, app->flagCount, app->pOptions, app->optionCount) ||
        !PRIV__picoCliValidateCommandList(app, app->pCommands, app->commandCount, 0) ||
        !PRIV__picoCliValidateAppCommandNames(app, app->pCommands, app->commandCount)) {
        return PRIV__picoCliHandleStatus(app, PICO_CLI_STATUS_ERROR);
    }

    context.pApp         = app;
    context.argc         = argc;
    context.argv         = argv;
    context.index        = argc > 0 ? 1 : 0;
    context.optionsEnded = false;
    status               = PRIV__picoCliParseScope(&context, NULL);
    if (status == PICO_CLI_STATUS_OK) {
        status = PRIV__picoCliFinishApp(&context);
    }
    return PRIV__picoCliHandleStatus(app, status);
}

#endif // PICO_CLI_IMPLEMENTATION
#endif // PICO_IMPLEMENTATION

#endif // PICO_CLI_H
