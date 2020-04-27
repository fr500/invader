/* User interface functions */
#ifndef KAMI_H_
#define KAMI_H_

#include <compat/strl.h>
#include <lists/string_list.h>
#include <string/stdstring.h>

#include "common.h"
#include "libretro/piccolo.h"
#include "util.h"

/*kami class is the class controls a core completely, provides the complete I/O for the core including file I/O, video,
 * audio, input. Implementation is GUI toolkit / paradygm specific*/
class Kami
{
private:
   /*variables*/
   PiccoloWrapper* piccolo;
   int current_core;
   int previous_core;
   int core_count;
   unsigned status;
   core_info_t* core_info;
   core_info_t core_info_list[100];
   const char* core_entries[100];
   core_frame_buffer_t* video_data;
   GLuint texture;

   bool core_loaded;

public:
   /*constructor*/
   Kami()
   {
      this->piccolo = new PiccoloWrapper();
      status = CORE_STATUS_NONE;
      current_core = 0;
      previous_core = -1;
      core_info = piccolo->get_info();
      core_count = 0;
      core_loaded = false;
   }
   void Main(const char* title);
   bool CoreListInit(const char* path);
   int RenderVideo();
   struct string_list* OptionGetValues(core_option_t* option);
   unsigned OptionGetIndex(core_option_t* option, struct string_list* values);
   void OptionUpdate(core_option_t* option, const char* value);
};

extern Kami* kami;
extern Kami* kami2;

bool kami_core_info_list_init(const char* in);

int kami_render_framebuffer(const core_frame_buffer_t* data, unsigned pixel_format);
size_t kami_render_audio(const int16_t* data, size_t frames);

bool kami_init_audio();

#endif