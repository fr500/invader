#include "piccolo.h"

static const char* tag = "[core]";

// pointer to the current instance
static Piccolo* piccolo_ptr;

unsigned frame = 0;

// set the current core instance
void Piccolo::set_instance_ptr(Piccolo* piccolo)
{
   piccolo_ptr = piccolo;
}

void Piccolo::core_get_variables(void* data)
{
   if (piccolo_ptr->option_count == 0)
      return;

   struct retro_variable* var = (struct retro_variable*)data;
   var->value = NULL;

   for (int i = 0; i < piccolo_ptr->option_count; i++)
   {
      if (!strcmp(var->key, piccolo_ptr->core_options[i].key))
         var->value = piccolo_ptr->core_options[i].value;
   }
}

void Piccolo::core_set_variables(void* data)
{
   char buf[PATH_MAX_LENGTH];
   const char* values;
   const char* value;

   piccolo_ptr->option_count = 0;

   struct retro_variable* vars = (struct retro_variable*)data;

   // pointer to count and iterate over options
   struct retro_variable* var = vars;

   // count core options
   while (var->key)
   {
      var++;
      piccolo_ptr->option_count++;
   }
   core_option_t* core_options = piccolo_ptr->core_options;

   logger(LOG_DEBUG, tag, "variables: %u\n", piccolo_ptr->option_count);

   for (unsigned i = 0; i < piccolo_ptr->option_count; i++)
   {
      unsigned j = 0;
      strlcpy(core_options[i].key, vars[i].key, sizeof(core_options[i].key));

      values = strstr(vars[i].value, "; ");
      value = values;

      while (values[j] != '|' && values[j])
      {
         value++;
         j++;
      }

      strlcpy(core_options[i].description, vars[i].value, values + 1 - vars[i].value);
      strlcpy(core_options[i].values, values + 2, sizeof(core_options[i].values));
      strlcpy(core_options[i].value, values + 2, value - values - 1);

      logger(
         LOG_DEBUG, tag, "key: %s description: %s values: %s default: %s\n", core_options[i].key,
         core_options[i].description, core_options[i].values, core_options[i].value);
   }
}

bool Piccolo::core_set_environment(unsigned cmd, void* data)
{
   switch (cmd)
   {
      case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
      {
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: %s\n", PRINT_BOOLEAN(*(bool*)data));
         piccolo_ptr->core_info.supports_no_game = *(bool*)data;
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_SET_VARIABLES:
      {
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_SET_VARIABLES:\n");
         piccolo_ptr->core_set_variables(data);
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_VARIABLE:
      {
         struct retro_variable* var = (struct retro_variable*)data;
         piccolo_ptr->core_get_variables(data);
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_VARIABLE: %s=%s\n", var->key, var->value);
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
      {
         if (piccolo_ptr->core_options)
            *(bool*)data = piccolo_ptr->options_updated;
         else
            *(bool*)data = false;
         if (piccolo_ptr->options_updated)
         {
            logger(
               LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: %s\n",
               piccolo_ptr->options_updated ? "true" : "false");
            piccolo_ptr->options_updated = false;
            return true;
         }
         else
            return false;

         break;
      }
      case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
      {
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: %s\n", PRINT_PIXFMT(*(int*)data));
         piccolo_ptr->core_info.pixel_format = *(int*)data;
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
      {
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_LOG_INTERFACE\n");
         struct retro_log_callback* callback = (struct retro_log_callback*)data;
         callback->log = internal_logger;
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
      {
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY\n", "./");
         *(const char**)data = "./";
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
      {
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY %s\n", "./");
         *(const char**)data = "./";
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:
      {
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY\n", "./");
         *(const char**)data = "./";
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
         *(unsigned*)data = 0;
         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: %d\n", *(unsigned*)data);
         return true;
         break;
      case RETRO_ENVIRONMENT_GET_CAN_DUPE:
      {
         *(bool*)data = true;
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
      {
         const input_descriptor_t* new_descriptors = NULL;
         unsigned count = 0;

         new_descriptors = (const input_descriptor_t*)data;

         for (count = 0; new_descriptors->description; new_descriptors++)
            count++;

         new_descriptors = (const input_descriptor_t*)data;
         piccolo_ptr->input_descriptors = (input_descriptor_t*)calloc(count, sizeof(input_descriptor_t));
         for (unsigned i = 0; i < count; i++)
         {
            piccolo_ptr->input_descriptors[i].port = new_descriptors[i].port;
            piccolo_ptr->input_descriptors[i].device = new_descriptors[i].device;
            piccolo_ptr->input_descriptors[i].id = new_descriptors[i].id;
            piccolo_ptr->input_descriptors[i].index = new_descriptors[i].index;
            piccolo_ptr->input_descriptors[i].description = new_descriptors[i].description;
         }
         piccolo_ptr->input_descriptors_size = count;
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
      {
         logger(LOG_DEBUG, tag, "RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL unhandled\n");
         return false;
         break;
      }
      case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
         logger(LOG_DEBUG, tag, "RETRO_ENVIRONMENT_GET_FASTFORWARDING unhandled\n");
         return false;
         break;
      case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
      {
         unsigned count = 0;
         controller_info_t* info = (controller_info_t*)data;
         for (count = 0; info[count].types; count++)
            ;

         logger(LOG_INFO, tag, "RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: controller ports: %d\n", count);
         controller_info_t* new_info = NULL;

         if (piccolo_ptr->controller_info)
            free(piccolo_ptr->controller_info);
         piccolo_ptr->controller_info = NULL;
         piccolo_ptr->controller_info_size = count;

         new_info = (controller_info_t*)calloc(count, sizeof(*new_info));
         if (!new_info)
            return false;

         piccolo_ptr->controller_info = new_info;
         memcpy(piccolo_ptr->controller_info, info, count * sizeof(*piccolo_ptr->controller_info));

         for (unsigned i = 0; info[i].types; i++)
         {
            for (unsigned j = 0; j < info[i].num_types; j++)
            {
               logger(
                  LOG_INFO, tag, "RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: creating device %s on port: %d\n",
                  piccolo_ptr->controller_info[i].types[j].desc, i + 1);
            }
         }
         return true;
         break;
      }
      case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
      {
         logger(
            LOG_INFO, tag, "RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: %s\n",
            piccolo_ptr->frontend_supports_bitmasks ? "true" : "false");
         return piccolo_ptr->frontend_supports_bitmasks;
         break;
      }
      case RETRO_ENVIRONMENT_SET_GEOMETRY:
      {
         struct retro_game_geometry* geometry = &piccolo_ptr->core_info.av_info.geometry;
         const struct retro_game_geometry* new_geometry = (const struct retro_game_geometry*)data;

         if (!geometry)
            return false;

         if (
            (geometry->base_width != new_geometry->base_width) || (geometry->base_height != new_geometry->base_height)
            || (geometry->aspect_ratio != new_geometry->aspect_ratio))
         {
            logger(
               LOG_INFO, tag, "RETRO_ENVIRONMENT_SET_GEOMETRY: old geometry: %dx%d (%f) new geometry: %dx%d (%f)\n",
               geometry->base_width, geometry->base_height, geometry->aspect_ratio, new_geometry->base_width,
               new_geometry->base_height, new_geometry->aspect_ratio);

            piccolo_ptr->core_info.av_info.geometry = *new_geometry;
         }
         break;
      }
      default:
         logger(LOG_DEBUG, tag, "unknown command: %d\n", cmd);
   }
   return false;
}

void Piccolo::core_input_poll()
{
   if (piccolo_ptr->poll_callback)
      piccolo_ptr->poll_callback();
   else
      logger(LOG_ERROR, tag, "input poll callback not set\n");

   return;
}

// TODO: analogs, keyboards, mice
int16_t Piccolo::core_input_state(unsigned port, unsigned device, unsigned index, unsigned id)
{
   // logger(
   //   LOG_DEBUG, tag, "frame: %d reading input for port: %d, device: %d, index: %d, id: %d\n", frame, port, device,
   //   index, id);
   input_state_t state = piccolo_ptr->input_state[port];
   int16_t ret;

   // buttons
   int16_t buttons = state.buttons;
   if (id == RETRO_DEVICE_ID_JOYPAD_MASK)
      ret = buttons;
   else
      ret = ((state.buttons & (1 << id)) >> id);

   return ret;
}

// TODO: hook this up
void Piccolo::core_audio_sample(int16_t left, int16_t right)
{
   return;
}

// TODO: hook this up
size_t Piccolo::core_audio_sample_batch(const int16_t* data, size_t frames)
{
   // return piccolo_ptr->audio_callback(data, frames);
   return 0;
}

void Piccolo::core_video_refresh(const void* data, unsigned width, unsigned height, size_t pitch)
{
   piccolo_ptr->video_data.data = data;
   piccolo_ptr->video_data.width = width;
   piccolo_ptr->video_data.height = height;
   piccolo_ptr->video_data.pitch = pitch;
   return;
}

bool Piccolo::load_game(const char* core_file_name, const char* game_file_name, bool peek)
{
   bool ret = false;
   status = CORE_STATUS_NONE;

   if (string_is_empty(core_file_name))
   {
      logger(LOG_ERROR, tag, "filename cannot be null\n");
      return ret;
   }

   option_count = 0;
   core_info.supports_no_game = false;
   core_info.block_extract = false;
   core_info.full_path = false;

   void (*set_environment)(retro_environment_t) = NULL;
   void (*set_video_refresh)(retro_video_refresh_t) = NULL;
   void (*set_input_poll)(retro_input_poll_t) = NULL;
   void (*set_input_state)(retro_input_state_t) = NULL;
   void (*set_audio_sample)(retro_audio_sample_t) = NULL;
   void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;

   library_handle = dylib_load(core_file_name);

   void (*proc)(struct retro_system_info*);
   proc = (void (*)(struct retro_system_info*))dylib_proc(library_handle, "retro_get_system_info");

   if (!library_handle)
   {
      logger(LOG_ERROR, tag, "failed to load library: %s\n", core_file_name);
      return false;
   }

   load_retro_sym(retro_api_version);
   load_retro_sym(retro_get_system_info);
   load_sym(set_environment, retro_set_environment);

   retro_api_version();
   retro_get_system_info(&system_info);

   strlcpy(core_info.file_name, core_file_name, sizeof(core_info.file_name));
   strlcpy(core_info.core_name, system_info.library_name, sizeof(core_info.core_name));
   strlcpy(core_info.core_version, system_info.library_version, sizeof(core_info.core_version));
   if (system_info.valid_extensions)
      strlcpy(core_info.extensions, system_info.valid_extensions, sizeof(core_info.extensions));
   else
      strlcpy(core_info.extensions, "N/A", sizeof(core_info.extensions));
   core_info.full_path = system_info.need_fullpath;
   core_info.block_extract = system_info.block_extract;

   logger(LOG_DEBUG, tag, "retro api version: %d\n", retro_api_version());
   logger(LOG_DEBUG, tag, "core name: %s\n", core_info.core_name);
   logger(LOG_DEBUG, tag, "core version: %s\n", core_info.core_version);
   logger(LOG_DEBUG, tag, "valid extensions: %s\n", core_info.extensions);

   set_environment(core_set_environment);

   if (peek)
   {
      dylib_close(library_handle);
      return true;
   }

   load_retro_sym(retro_init);
   load_retro_sym(retro_deinit);
   load_retro_sym(retro_get_system_av_info);
   load_retro_sym(retro_set_controller_port_device);
   load_retro_sym(retro_reset);
   load_retro_sym(retro_run);
   load_retro_sym(retro_load_game);
   load_retro_sym(retro_unload_game);
   load_retro_sym(retro_get_memory_data);
   load_retro_sym(retro_get_memory_size);

   load_retro_sym(retro_serialize);
   load_retro_sym(retro_serialize_size);
   load_retro_sym(retro_unserialize);

   load_sym(set_video_refresh, retro_set_video_refresh);
   load_sym(set_input_poll, retro_set_input_poll);
   load_sym(set_input_state, retro_set_input_state);
   load_sym(set_audio_sample, retro_set_audio_sample);
   load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);

   retro_init();

   set_video_refresh(core_video_refresh);
   set_input_poll(core_input_poll);
   set_input_state(core_input_state);
   set_audio_sample(core_audio_sample);
   set_audio_sample_batch(core_audio_sample_batch);

   status = CORE_STATUS_LOADED;

   // supports no-game codepath
   if (!game_file_name)
   {
      if (retro_load_game(NULL))
      {
         logger(LOG_INFO, tag, "loading without content\n");
         ret = true;
      }
      else
      {
         logger(LOG_ERROR, tag, "loading failed\n");
         ret = false;
      }
   }
   else
   {
      if (core_info.full_path)
      {
         struct retro_game_info info;
         info.data = NULL;
         info.size = 0;
         info.path = game_file_name;
         logger(LOG_INFO, tag, "loading file %s\n", info.path);
         if (!retro_load_game(&info))
            logger(LOG_ERROR, tag, "core error while opening file %s\n", game_file_name);
         else
            ret = true;
      }
      else
      {
         struct retro_game_info info;
         FILE* file = fopen(game_file_name, "rb");
         if (!file)
            logger(LOG_ERROR, tag, "error opening file %s\n", game_file_name);
         fseek(file, 0, SEEK_END);
         info.size = ftell(file);
         rewind(file);

         info.path = game_file_name;
         info.data = calloc(1, info.size);

         if (!info.data || !fread((void*)info.data, info.size, 1, file))
            logger(LOG_ERROR, tag, "error reading file %s\n", game_file_name);
         if (!retro_load_game(&info))
            logger(LOG_ERROR, tag, "core error while opening file %s\n", game_file_name);
         else
            ret = true;
      }
   }

   retro_get_system_av_info(&core_info.av_info);

   logger(
      LOG_DEBUG, tag, "geometry: %ux%d/%ux%d %f\n", core_info.av_info.geometry.base_width,
      core_info.av_info.geometry.base_height, core_info.av_info.geometry.max_width,
      core_info.av_info.geometry.max_height, core_info.av_info.geometry.aspect_ratio);
   logger(LOG_DEBUG, tag, "timing: %ffps %fHz\n", core_info.av_info.timing.fps, core_info.av_info.timing.sample_rate);

   return ret;
}

void Piccolo::core_run(audio_cb_t cb)
{
   if (status != CORE_STATUS_RUNNING)
      status = CORE_STATUS_RUNNING;
   piccolo_ptr->retro_run();
   frame++;
}

void Piccolo::core_reset()
{
   if (status == CORE_STATUS_RUNNING)
      piccolo_ptr->retro_reset();
   else
      return;
}