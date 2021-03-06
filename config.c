/*########################
###
###                This Programm
###                falls under the
###                MIT Software
###                license. Check
###                COPYING for
###                further
###                information.
###
########################*/

#include <string.h>

#include "config.h"
#include "universal.h"
//#include "building.h"

ConfigEntry configdefs[] = {
        {BS_KEYBINDING,                         KEY_UP,                         "key_up"},
        {BS_KEYBINDING,                         KEY_DOWN,                       "key_down"},
        {BS_KEYBINDING,                         KEY_LEFT,                       "key_left"},
        {BS_KEYBINDING,                         KEY_RIGHT,                      "key_right"},

        {BS_KEYBINDING,                         KEY_SHOT,                       "key_shot"},

        {BS_KEYBINDING,                         KEY_FULLSCREEN,                 "key_fullscreen"},
        {BS_KEYBINDING,                         KEY_MENU,                       "key_menu"},

        {BS_INT,                                FULLSCREEN,                     "fullscreen"},
        {BS_INT,                                NO_SHADER,                      "disable_shader"},
        {BS_INT,                                NO_AUDIO,                       "disable_audio"},
        {BS_INT,                                NO_STAGEBG,                     "disable_stagebg"},
        {BS_INT,                                NO_STAGEBG_FPSLIMIT,            "disable_stagebg_auto_fpslimit"},
        {BS_INT,                                GFX_WIDTH,                      "vid_width"},
        {BS_INT,                                GFX_HEIGHT,                     "vid_height"},
        {BS_STRING,                             PLAYERNAME,                     "playername"},

        {0, 0, 0}
};

ConfigEntry* config_findentry(char *name) {
        ConfigEntry *e = configdefs;
        do if(!strcmp(e->name, name)) return e; while((++e)->name);
        return NULL;
}

ConfigEntry* config_findentry_byid(int id) {
        ConfigEntry *e = configdefs;
        do if(id == e->key) return e; while((++e)->name);
        return NULL;
}

void config_preset(void) {
        memset(configdata.strval, 0, sizeof(configdata.strval));
        memset(configdata.intval, 0, sizeof(configdata.intval));

        configdata.intval[KEY_UP] = SDLK_UP;
        configdata.intval[KEY_DOWN] = SDLK_DOWN;
        configdata.intval[KEY_LEFT] = SDLK_LEFT;
        configdata.intval[KEY_RIGHT] = SDLK_RIGHT;

        configdata.intval[KEY_SHOT] = SDLK_RETURN;

        configdata.intval[KEY_FULLSCREEN] = SDLK_F11;
        configdata.intval[KEY_MENU] = SDLK_ESCAPE;

        configdata.intval[FULLSCREEN] = 0;

        //configdata.intval[NO_SHADER] = 0;
        //configdata.intval[NO_AUDIO] = 0; (for future relevance)

        configdata.intval[GFX_WIDTH] = RESOLUTIONX;
        configdata.intval[GFX_HEIGHT] = RESOLUTIONY;

        configdata.intval[FIELDW] = 13;
        configdata.intval[FIELDH] = 13;

        char *name = "Player";
        configdata.strval[PLAYERNAME] = malloc(strlen(name)+1);
        strcpy(configdata.strval[PLAYERNAME], name);
}

int config_sym2key(int sym) {
        int i;
        for(i = CONFIG_KEY_FIRST; i <= CONFIG_KEY_LAST; ++i)
                if(sym == configdata.intval[i])
                        return i;
        return -1;
}

FILE* open_config(char *filename, char *mode) {
        char *buffer;
        FILE *output;

        buffer = malloc(strlen(filename) + strlen(get_configuration_path()) + 2);
        strcpy(buffer, get_configuration_path());
        strcat(buffer, "/");
        strcat(buffer, filename);

        output = fopen(buffer, mode);
        free(buffer);

        if(!output) {
                printf("open_config(): couldn't open '%s'", filename);
                return NULL;
        }

        return output;
}

int config_intval_p(ConfigEntry *e) {
        return configdata.intval[e->key];
}

char* config_strval_p(ConfigEntry *e) {
        return configdata.strval[e->key];
}

int config_intval(char *key) {
        return config_intval_p(config_findentry(key));
}

char* config_strval(char *key) {
        return config_strval_p(config_findentry(key));
}

void config_save(char *filename) {
        FILE *output = open_config(filename, "w");
        ConfigEntry *e = configdefs;

        if(!output)
                return;

        do switch(e->type) {
                case CFGT_INT:
                        fprintf(output, "%s = %i\n", e->name, config_intval_p(e));
                        break;

                case CFGT_KEYBINDING:
                        fprintf(output, "%s = K%i\n", e->name, config_intval_p(e));
                        break;

                case CFGT_STRING:
                        fprintf(output, "%s = %s\n", e->name, config_strval_p(e));
                        break;

        } while((++e)->name);

        fclose(out);
        printf("config_save(): Saved config '%s'\n", filename);
}


#define INTOF(s)   ((int)strtol(s, NULL, 10))

void set_config(char *key, char *val) {
        ConfigEntry *e = findentry_config(key);

        if(!e) {
                printf("config_set(): unknown key '%s'", key);
                return;
        }

        switch(e->type) {
                case CFGT_INT:
                        configdata.intval[e->key] = INTOF(val);
                        break;

                case CFGT_KEYBINDING:
                        configdata.intval[e->key] = INTOF(val+1);
                        break;

                case CFGT_STRING:
                        stralloc(&(configdata.strval[e->key]), val);
                        break;
        }
}

#undef INTOF

#define SYNTAXERROR { warnx("config_load(): syntax error on line %i, aborted! [%s:%i]\n", line, __FILE__, __LINE__); goto end; }
#define BUFFERERROR { warnx("config_load(): string exceed the limit of %i, aborted! [%s:%i]", CONFIG_LOAD_BUFSIZE, __FILE__, __LINE__); goto end; }

void load_config(char *filename) {
        FILE *input = open_config(filename, "r");
        int c, i = 0, found, line = 0;
        char buffer[CONFIG_LOAD_BUFFERSIZE];
        char key[CONFIG_LOAD_BUFFERSIZE];
        char val[CONFIG_LOAD_BUFFERSIZE];

        config_preset();
        if(!input) {
                printf("Configfile either not readable or existing.");
                return;
        }

        while((c = fgetc(input)) != EOF) {
                if(c == '#' && !i) {
                        i = 0;
                        while(fgetc(input) != '\n');
                } else if(c == ' ') {
                        if(!i) SYNTAXERROR

                        buffer[i] = 0;
                        i = 0;
                        strcpy(key, buffer);

                        found = 0;
                        while((c = fgetc(input)) != EOF) {
                                if(c == '=') {
                                        if(++found > 1) SYNTAXERROR
                                } else if(c != ' ') {
                                        if(!found || c == '\n') SYNTAXERROR

                                        do {
                                                if(c == '\n') {
                                                        if(!i) SYNTAXERROR

                                                        buffer[i] = 0;
                                                        i = 0;
                                                        strcpy(val, buffer);
                                                        found = 0;
                                                        ++line;

                                                        config_set(key, val);
                                                        break;
                                                } else {
                                                        buffer[i++] = c;
                                                        if(i == CONFIG_LOAD_BUFFERSIZE)
                                                                BUFFERERROR
                                                }
                                        } while((c = fgetc(input)) != EOF);

                                        break;
                                }
                        } if(found) SYNTAXERROR
                } else {
                        buffer[i++] = c;
                        if(i == CONFIG_LOAD_BUFFERSIZE)
                                BUFFERERROR
                }
        }

end:
        fclose(input);
}

#undef SYNTAXERROR
#undef BUFFERERROR
