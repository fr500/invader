#ifndef PICCOLO_H_
#define PICCOLO_H_

// libretro common
extern "C" {
#include <dynamic/dylib.h>
}
#include "libretro.h"

#include "util.h"

extern "C" {
dylib_t dylib_load(const char* path);
void dylib_close(dylib_t lib);
char* dylib_error(void);
function_t dylib_proc(dylib_t lib, const char* proc);
}

#define MAX_PORTS 16
#define MAX_IDS 12

#define load_sym(V, S) \
   do \
   { \
      function_t func = dylib_proc(this->library_handle, #S); \
      memcpy(&V, &func, sizeof(func)); \
      if (!func) \
         logger(LOG_ERROR, tag, "failed to load symbol '" #S "'': %s"); \
   } while (0)
#define load_retro_sym(S) load_sym(this->S, S)

static void internal_logger(enum retro_log_level level, const char* fmt, ...)
{
#ifndef DEBUG
   if (level == RETRO_LOG_DEBUG)
      return;
#endif
   va_list va;
   char buffer[4096] = {0};
   static const char* level_char = "diwe";

   va_start(va, fmt);
   vsnprintf(buffer, sizeof(buffer), fmt, va);
   va_end(va);

   fprintf(stderr, "[%c] --- %s %s", level_char[level], "[libretro]", buffer);
   fflush(stderr);
}

// core frame buffer data
typedef struct core_frame_buffer
{
   const void* data;
   unsigned width;
   unsigned height;
   unsigned pitch;
} core_frame_buffer_t;

// audio callback
typedef size_t (*audio_cb_t)(const int16_t*, size_t);

// input poll callback
typedef void (*input_poll_t)();

// core information
typedef struct core_info
{
   char file_name[PATH_MAX_LENGTH];
   char core_name[PATH_MAX_LENGTH];
   char core_version[PATH_MAX_LENGTH];
   char extensions[PATH_MAX_LENGTH];

   bool supports_no_game;
   bool block_extract;
   bool full_path;

   unsigned pixel_format;

   struct retro_system_av_info av_info;
} core_info_t;

// core options
typedef struct core_option
{
   char key[100];
   char description[100];
   char value[100];
   char values[PATH_MAX_LENGTH];
} core_option_t;

// controller info
typedef struct retro_controller_info controller_info_t;
typedef struct retro_controller_description controller_description_t;

// input state
typedef struct
{
   int16_t buttons;
   uint16_t analogs[8];
   uint16_t analog_buttons[16];
} input_state_t;

enum core_status
{
   CORE_STATUS_NONE = 0,
   CORE_STATUS_LOADED,
   CORE_STATUS_RUNNING
};

// button descriptors
typedef struct retro_input_descriptor input_descriptor_t;

// piccolo class is the class that interacts with the libretro core directly
class Piccolo
{
private:
   // variables
   void* library_handle;
   unsigned status;
   bool options_updated;
   bool frontend_supports_bitmasks;
   size_t option_count;

   core_option_t core_options[1000];
   core_info_t core_info;
   core_frame_buffer_t video_data;
   audio_cb_t audio_callback;

   input_poll_t poll_callback;

   input_state_t input_state[MAX_PORTS];

   controller_info_t* controller_info;
   size_t controller_info_size;

   input_descriptor_t* input_descriptors;
   size_t input_descriptors_size;

   int controller_port_device[MAX_PORTS];

   // libretro variables
   struct retro_system_info system_info;

   // libretro functions
   void (*retro_init)(void);
   void (*retro_deinit)(void);
   unsigned (*retro_api_version)(void);
   void (*retro_get_system_info)(struct retro_system_info* info);
   void (*retro_get_system_av_info)(struct retro_system_av_info* info);
   void (*retro_set_controller_port_device)(unsigned port, unsigned device);
   void (*retro_reset)(void);
   void (*retro_run)(void);
   size_t (*retro_serialize_size)(void);
   bool (*retro_serialize)(void* data, size_t size);
   bool (*retro_unserialize)(const void* data, size_t size);
   bool (*retro_load_game)(const struct retro_game_info* game);
   void (*retro_unload_game)(void);
   void* (*retro_get_memory_data)(unsigned id);
   size_t (*retro_get_memory_size)(unsigned id);

   // internal helper functions
   static void core_set_variables(void* data);
   static void core_get_variables(void* data);
   static void core_video_refresh(const void* data, unsigned width, unsigned height, size_t pitch);
   static void core_input_poll();
   static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id);
   static void core_audio_sample(int16_t left, int16_t right);
   static size_t core_audio_sample_batch(const int16_t* data, size_t frames);
   static bool core_set_environment(unsigned cmd, void* data);

public:
   // constructor
   Piccolo() { }
   ~Piccolo() { }

   // helper functions
   // load game
   bool load_game(const char* core_file_name, const char* game_file_name, bool peek);
   // core run
   void core_run(audio_cb_t cb);
   // core reset
   void core_reset();

   // accessors
   // get core information
   core_info_t* get_info() { return &core_info; }
   // get core options array
   core_option_t* get_options() { return core_options; }
   // get core options count
   size_t get_option_count() { return option_count; }
   // get video data
   void set_options_updated() { options_updated = true; }
   // get core status
   unsigned get_status() { return status; }
   // get video data
   core_frame_buffer_t* get_video_data() { return &video_data; }
   // get input port info
   controller_info_t* get_controller_info() { return controller_info; }
   // get input port count
   size_t get_controller_port_count() { return controller_info_size; }
   // set device in port
   void set_controller_port_device(int port, int device)
   {
      controller_port_device[port] = device;
      retro_set_controller_port_device(port, device);
   }
   // get input descriptors
   input_descriptor_t* get_input_descriptors() { return input_descriptors; }
   // get the count of set input descriptors
   size_t get_input_descriptor_count() { return input_descriptors_size; }
   // set callbacks for stuff that is handled in the frontend
   void set_callbacks(input_poll_t cb) { poll_callback = cb; }
   // set input state
   void set_input_state(unsigned port, input_state_t state)
   {
      input_state[port].buttons = state.buttons;

      for (unsigned i = 0; i < 8; i++)
         input_state[port].analogs[i] = state.analogs[i];
   }
   // set support bitmasks
   void set_frontend_supports_bitmasks(bool value) { frontend_supports_bitmasks = value; }
   // set the current core instance
   void set_instance_ptr(Piccolo* piccolo);
};

// piccolo controller is a wrapper for piccolo with the single purpose of making sure the correct instance pointer is
// set in every function call to achieve multi - instancing
class PiccoloWrapper
{
private:
   Piccolo* piccolo;

public:
   // constructor
   PiccoloWrapper() { }
   // destructor
   ~PiccoloWrapper() { piccolo->set_instance_ptr(piccolo); }

   // load core for use
   bool load_game(const char* core_file_name, const char* game_file_name, bool bitmasks)
   {
      piccolo->set_instance_ptr(piccolo);
      piccolo->set_frontend_supports_bitmasks(bitmasks);
      return piccolo->load_game(core_file_name, game_file_name, false);
   }
   // load core to peek for core information
   bool peek_core(const char* core_file_name)
   {
      piccolo = new Piccolo();
      piccolo->set_instance_ptr(piccolo);
      return piccolo->load_game(core_file_name, NULL, true);
   }
   // core run
   void core_run(audio_cb_t cb)
   {
      piccolo->set_instance_ptr(piccolo);
      piccolo->core_run(cb);
   }
   // core reset
   void core_reset()
   {
      piccolo->set_instance_ptr(piccolo);
      piccolo->core_reset();
   }

   // accessors
   // get core information
   core_info_t* get_info()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_info();
   }
   // get core options array
   core_option_t* get_options()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_options();
   }
   // get core options count
   size_t get_option_count()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_option_count();
   }
   // get video data
   void set_options_updated()
   {
      piccolo->set_instance_ptr(piccolo);
      piccolo->set_options_updated();
   }
   // get core status
   unsigned get_status()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_status();
   }
   // get video data
   core_frame_buffer_t* get_video_data()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_video_data();
   };
   // get input port info
   controller_info_t* get_controller_info()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_controller_info();
   }
   // get input port count
   size_t get_controller_port_count()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_controller_port_count();
   }
   // set device in port
   void set_controller_port_device(int port, int device)
   {
      piccolo->set_instance_ptr(piccolo);
      piccolo->set_controller_port_device(port, device);
   }
   // get input descriptors
   input_descriptor_t* get_input_descriptors()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_input_descriptors();
   }
   // get the count of set input descriptors
   size_t get_input_descriptor_count()
   {
      piccolo->set_instance_ptr(piccolo);
      return piccolo->get_input_descriptor_count();
   }
   // set callbacks for stuff that is handled in the frontend
   void set_callbacks(input_poll_t cb)
   {
      piccolo = new Piccolo();
      piccolo->set_instance_ptr(piccolo);
      piccolo->set_callbacks(cb);
   }
   // set input state
   void set_input_state(unsigned port, input_state_t state) { piccolo->set_input_state(port, state); }

   // core deinit
   void unload_core()
   {
      piccolo->set_instance_ptr(piccolo);
      // TODO: hookup actual core unloading
      delete piccolo;
   }
};

#endif