#include "config.h"
#include "ini.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <compat/strl.h>
#include <string/stdstring.h>

#include <libintl.h>
#include <locale.h>

#include <file/file_path.h>

#include "util.h"

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static const char* tag = "[config]";

const char* setting_category_labels[] =
{
   "setting_categories_none",
   "setting_categories_general",
   "setting_categories_video",
   "setting_categories_audio",
   "setting_categories_input",
   "setting_categories_paths",
};

settings_t* settings_list;

settings_t* settings_get()
{
   return settings_list;
}

void settings_push(settings_t* head, setting_t* new) {
   settings_t * current = head;

   if (head->data == NULL)
   {
      current->data = new;
      return;
   }
   while (current->next != NULL)
      current = current->next;

   /* now we can add a new variable */
   current->next = (settings_t *) calloc(1, sizeof(settings_t));
   current->next->data = new;
   current->next->next = NULL;
}


void setting_init_string(setting_t* s, unsigned category, char* name, size_t size)
{
   s->type = SETTING_STRING;
   s->size = size;
   s->categories = category;
   s->data = (char *)calloc(s->size, sizeof(char));
   strlcpy(s->name, name, sizeof(s->name));
}

void setting_init_bool(setting_t* s, unsigned category, char* name)
{
   s->type = SETTING_BOOL;
   s->size = sizeof(bool);
   s->categories = category;
   s->data = (char *)calloc(s->size, sizeof(bool));
   strlcpy(s->name, name, sizeof(s->name));
}

static int setting_definitions_handler(void* c, const char* section,
   const char* name, const char* value)
{
   static char setting_name[50] = "";

   static unsigned type;
   static unsigned flags;
   static unsigned categories;
   static size_t size;
   static float min, max, step;

   static unsigned index;

   if (!string_is_empty(section))
   {
      if (string_is_empty(setting_name) || string_is_equal(section, setting_name))
      {
         if(string_is_equal(name, "type"))
            type = atoi(value);
         else if(string_is_equal(name, "categories"))
            categories = atoi(value);
         else if(string_is_equal(name, "size"))
            size = atoi(value);
         else if(string_is_equal(name, "min"))
            min = atoi(value);
         else if(string_is_equal(name, "max"))
            max = atoi(value);
         else if(string_is_equal(name, "step"))
            step = atoi(value);
      }
      else
      {
         logger(LOG_DEBUG, tag, "creating setting: %s\n", setting_name);
         static int index = 0;
         setting_t* s = (setting_t *)calloc(1, sizeof(setting_t));;
         switch(type)
         {
            case SETTING_BOOL:
               setting_init_bool(s, categories, setting_name);
               break;
            case SETTING_STRING:
               setting_init_string(s, categories, setting_name, size);
               break;
            default:
               break;
         }
         settings_push(settings_list, s);
      }

      strlcpy(setting_name, section, sizeof(setting_name));
   }
   else
      logger(LOG_DEBUG, tag, "empty sections unsupported on definitions file\n", name);
   return true;
}

static int config_load_handler(void* c, const char* section,
   const char* name, const char* value)
{
   if (string_is_empty(section))
   {
      setting_t* s = setting_get((char *)name);
      logger(LOG_DEBUG, tag, "settings %s label: %s\n", s->name, setting_get_label(s));
      logger(LOG_DEBUG, tag, "settings %s description: %s\n", s->name, setting_get_desc(s));

      switch(s->type)
      {
         case SETTING_INT:
         case SETTING_UINT:
            *((unsigned*)s->data) = atoi(value);
            logger(LOG_DEBUG, tag, "settings %s value: %d\n", s->name, *((unsigned*)s->data));
            break;
         case SETTING_FLOAT:
            *((float*)s->data) = atof(value);
            logger(LOG_DEBUG, tag, "settings %s value: %d\n", s->name, *((float*)s->data));
            break;
         case SETTING_BOOL:
            if (!strcmp(value, "true"))
               *((bool*)s->data) = true;
            else
               *((bool*)s->data) = false;
            logger(LOG_DEBUG, tag, "settings %s value: %s\n", s->name, *((bool*)s->data) ? "true" : "false");
            break;
         case SETTING_STRING:
            strlcpy(s->data, value, s->size);
            logger(LOG_INFO, tag,  "settings %s value: %s size: %d\n", s->name, s->data, s->size);
            break;
         default:
            logger(LOG_DEBUG, tag, "settings %s unknown\n", s->name);
      }
   }
   else
      logger(LOG_DEBUG, tag, "section %s unknown\n", name);
   return true;
}

const char* setting_get_label(setting_t* s)
{
   char label[60];

   strlcpy(label, s->name, sizeof(label));
   strlcat(label, "_label", sizeof(label));

   return __(label);
}

const char* setting_get_desc(setting_t* s)
{
   char desc[60];

   strlcpy(desc, s->name, sizeof(desc));
   strlcat(desc, "_desc", sizeof(desc));

   return __(desc);
}

const char* category_label(unsigned category)
{
   return __(setting_category_labels[category]);
}

setting_t* setting_get(char* s)
{
   settings_t* current = settings_list;
   while (current->next)
   {
      if (!strcmp(current->data->name, s))
         return current->data;
      else
         current = current->next;
   }
   return NULL;
}

unsigned* setting_uint_val(char* s)
{
   return ((unsigned*)setting_get(s)->data);
}

float* setting_float_val(char* s)
{
   return ((float*)setting_get(s)->data);
}

bool* setting_bool_val(char* s)
{
   return ((bool*)setting_get(s)->data);
}

char* setting_string_val(char* s)
{
   return ((char*)setting_get(s)->data);
}

bool config_load(char* file)
{
   setting_t* s;
   char definitions[PATH_MAX], buf[PATH_MAX];

   settings_list = (settings_t *)calloc(1, sizeof(settings_t));

   strlcpy(buf, file, sizeof(buf));
   path_remove_extension(buf);
   snprintf(definitions, sizeof(definitions), "%s.def", buf);

   logger(LOG_INFO, tag, "loading setting definitions: %s\n", definitions);
   if (ini_parse(definitions, setting_definitions_handler, NULL) < 0)
   {
      logger(LOG_ERROR, tag, "error loading config file: %s\n", definitions);
      return false;
   }


   logger(LOG_INFO, tag, "loading config file: %s\n", file);
   if (ini_parse(file, config_load_handler, NULL) < 0)
   {
      logger(LOG_ERROR, tag, "error loading config file: %s\n", file);
      return false;
   }

   fflush(stdout);
   return true;
}