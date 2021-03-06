#include "imgui.h"

#include "settings.h"
#include "widgets.h"

// settings widgets

bool Setting<bool>::Render()
{
   std::string label = m_name + "_label";
   std::string desc = m_name + "_desc";

   bool ret = ImGui::Checkbox(_(label.c_str()), &m_value);
   Widgets::Tooltip(_(desc.c_str()));

   if (ret)
      setting_event();

   return ret;
}

bool Setting<scale_mode_t>::Render()
{
   std::string label = m_name + "_label";
   std::string desc = m_name + "_desc";

   char** entries = (char**)calloc(SCALE_MODE_LAST, sizeof(scale_mode_t*));
   for (unsigned i = 0; i < SCALE_MODE_LAST; i++)
   {
      std::string entry = scale_modes[i].m_name + "_label";
      char* cstr = &entry[0];
      entries[i] = _(cstr);
   }

   bool ret = ImGui::Combo(_(label.c_str()), &m_value.m_mode, entries, SCALE_MODE_LAST);
   Widgets::Tooltip(_(desc.c_str()));

   return ret;
}