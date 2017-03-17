/*
Copyright (c) 2017, Mike Tzou
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of foo_uie_dsp_switcher nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdafx.h"

#include "main.h"

DECLARE_COMPONENT_VERSION
(
	PLUGIN_NAME, PLUGIN_VERSION,

	PLUGIN_NAME"\n"
	"Compiled on: "__DATE__"\n"
	"Linked Columns UI version: " UI_EXTENSION_VERSION "\n"
	"\n"
	"https://github.com/Chocobo1/foo_uie_dsp_switcher\n"
	"This plugin is released under BSD 3-Clause license\n"
	"Mike Tzou (Chocobo1)\n"
);


#define ARRAY_LENGTH(a) (std::extent<decltype(a)>::value)

#define WM_USER_DSP_CORE_CHANGE (WM_USER + 0)


void DspSwitcher::initCombobox()
{
	// get dsp presets names
	ItemList list = menuFindCommands();
	if (m_comboboxEntries == list)
		return;

	m_comboboxEntries = list;

	// clear existing entries
	addToCombobox("");  // ensure a 1-item height empty item-list is displayed
	clearCombobox();

	for (t_size i = 0, imax = m_comboboxEntries.get_count(); i < imax; ++i)
	{
		addToCombobox(m_comboboxEntries[i].name.c_str());

		if (m_comboboxEntries[i].selected)
			selectItem(i);
	}

	// determine width
	// MSDN Remarks: After an application has finished drawing with the new font object , it should always replace a new font object with the original font object.
	m_minSizeWidth = INT_MAX;
	m_fullSizeWidth = 0;
	CONST HDC dc = ::GetDC(m_combobox);
	CONST HFONT origFont = SelectFont(dc, m_uiHfont);
	for (t_size i = 0, imax = m_comboboxEntries.get_count(); i < imax; ++i)
	{
		const int cx = ui_helpers::get_text_width(dc, m_comboboxEntries[i].name.c_str(), m_comboboxEntries[i].name.length());
		m_minSizeWidth = min(m_minSizeWidth, cx);
		m_fullSizeWidth = max(m_fullSizeWidth, cx);
	}
	m_extraMargin = ui_helpers::get_text_width(dc, "0", 1);
	SelectFont(dc, origFont);
	ReleaseDC(m_combobox, dc);

	// get min width
	COMBOBOXINFO cbi = {0};
	cbi.cbSize = sizeof(cbi);
	GetComboBoxInfo(m_combobox, &cbi);

	RECT rcClient;
	GetClientRect(m_combobox, &rcClient);
	const long rectMargin = RECT_CX(rcClient) - RECT_CX(cbi.rcItem);
	m_minSizeWidth += rectMargin;
	m_fullSizeWidth += rectMargin;

	return;
}


void DspSwitcher::clearCombobox() const
{
	CONST LRESULT ret = uSendMessage(m_combobox, CB_RESETCONTENT, 0, 0);
	if (ret < 0)
		console::printf(CONSOLE_HEADER "%s() failed", __FUNCTION__);
}


bool DspSwitcher::addToCombobox(const char *str) const
{
	CONST LRESULT ret = uSendMessageText(m_combobox, CB_ADDSTRING, NULL, str);
	if (ret < 0)
	{
		console::printf(CONSOLE_HEADER "%s() failed", __FUNCTION__);
		return false;
	}
	return true;
}


bool DspSwitcher::selectItem(const int idx) const
{
	CONST LRESULT ret = uSendMessage(m_combobox, CB_SETCURSEL, idx, NULL);
	if (ret < 0)
	{
		console::printf(CONSOLE_HEADER "%s() failed", __FUNCTION__);
		return false;
	}
	return true;
}


bool DspSwitcher::getSelectItem(DspSwitcher::ComboboxItem &out) const
{
	int idx = uSendMessage(m_combobox, CB_GETCURSEL, 0, 0);
	if (idx == CB_ERR)
	{
		console::printf(CONSOLE_HEADER "%s() failed", __FUNCTION__);
		return false;
	}

	out = m_comboboxEntries[idx];
	return true;
}


bool DspSwitcher::setComboboxWidth(const int width) const
{
	int idx = uSendMessage(m_combobox, CB_SETDROPPEDWIDTH, width, 0);
	if (idx == CB_ERR)
	{
		console::printf(CONSOLE_HEADER "%s() failed", __FUNCTION__);
		return false;
	}

	return true;
}


DspSwitcher::ItemList DspSwitcher::menuFindCommands() const
{
	// dynamic menu item: (Shift + Menu) -> Playback -> DSP settings -> Preferences...
	const GUID PREFERENCES_GUID = {0xec9d5746, 0xcede, 0x454e, {0x84,0xc4,0xa7,0xad,0xa1,0xc7,0x7f,0x9a}};

	ItemList out;

	// enumerate mainmenu items
	service_enum_t<mainmenu_commands> e;
	service_ptr_t<mainmenu_commands_v2> ptr;
	while (e.next(ptr))
	{
		for (t_uint32 i = 0, imax = ptr->get_command_count(); i < imax; ++i)
		{
			// should be a dynamic item
			if (!ptr->is_command_dynamic(i))
			{
				//console::printf(CONSOLE_HEADER "%s(): item is NOT dynamic!!", __FUNCTION__);
				continue;
			}
			const mainmenu_node::ptr targetGroupNode = ptr->dynamic_instantiate(i);

			// should be a group node
			if (targetGroupNode->get_type() != mainmenu_node::type_group)
			{
				//console::printf(CONSOLE_HEADER "%s(): node is NOT type_group!!", __FUNCTION__);
				continue;
			}

			// check if this is the right group
			const t_size childrenCount = targetGroupNode->get_children_count();
			const GUID lastChildrenGUID = targetGroupNode->get_child(childrenCount - 1)->get_guid();
			if (lastChildrenGUID != PREFERENCES_GUID)
			{
				//console::printf(CONSOLE_HEADER "%s(): last child is not our target GUID!!", __FUNCTION__);
				continue;
			}

			// finally, our targets
			for (t_size j = 0, jmax = (childrenCount - 1); j < jmax; ++j)  // skip over the last item
			{
				const mainmenu_node::ptr itemNode = targetGroupNode->get_child(j);
				if (itemNode->get_type() != mainmenu_node::type_command)
					continue;

				pfc::string8 name;
				t_uint32 flags;
				itemNode->get_display(name, flags);
				const bool isSelected = (flags & mainmenu_commands::flag_radiochecked) > 0;
				out.add_item({name, itemNode->get_guid(), isSelected});

				//console::printf(CONSOLE_HEADER "%s", name.toString());
			}
			return out;
		}
	}

	return out;
}


void DspSwitcher::menuExecCommand(const GUID &commandGuid)
{
	service_enum_t<mainmenu_commands> e;
	service_ptr_t<mainmenu_commands_v2> ptr;
	while (e.next(ptr))
	{
		for (t_uint32 i = 0, imax = ptr->get_command_count(); i < imax; ++i)
		{
			// should be a dynamic item
			if (!ptr->is_command_dynamic(i))
			{
				//console::printf(CONSOLE_HEADER "%s(): item is NOT dynamic!!", __FUNCTION__);
				continue;
			}
			const mainmenu_node::ptr targetGroupNode = ptr->dynamic_instantiate(i);

			// should be a group node
			if (targetGroupNode->get_type() != mainmenu_node::type_group)
			{
				//console::printf(CONSOLE_HEADER "%s(): node is NOT type_group!!", __FUNCTION__);
				continue;
			}

			// iterate over children
			for (t_size j = 0, jmax = targetGroupNode->get_children_count(); j < jmax; ++j)
			{
				const mainmenu_node::ptr node = targetGroupNode->get_child(j);
				if (node->get_guid() != commandGuid)
					continue;

				node->execute(nullptr);
				return;
			}
		}
	}
}


LRESULT DspSwitcher::on_message(const HWND parentWnd, const UINT msg, const WPARAM wp, const LPARAM lp)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			//console::printf(CONSOLE_HEADER "got WM_CREATE");

			if (m_uiHfont != NULL)
			{
				console::printf(CONSOLE_HEADER "Error: m_uiHfont != NULL");
				return -1;
			}
			if (m_combobox != NULL)
			{
				console::printf(CONSOLE_HEADER "Error: m_combobox != NULL");
				return -1;
			}
			if (m_toolTip != NULL)
			{
				console::printf(CONSOLE_HEADER "Error: m_toolTip != NULL");
				return -1;
			}

			// get columns UI font
			m_uiHfont = uCreateIconFont();
			if (m_uiHfont == NULL)
			{
				console::printf(CONSOLE_HEADER "uCreateIconFont() failed");
				return -1;
			}

			m_combobox = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, (CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_TABSTOP), CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentWnd, NULL, core_api::get_my_instance(), nullptr);
			if (m_combobox == NULL)
			{
				console::printf(CONSOLE_HEADER "m_combobox = CreateWindowEx() failed");

				::DeleteFont(m_uiHfont);
				m_uiHfont = NULL;
				return -1;
			}

			m_toolTip = CreateWindowEx(0, TOOLTIPS_CLASS, nullptr, (WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP), CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentWnd, NULL, core_api::get_my_instance(), nullptr);
			if (m_toolTip == NULL)
			{
				console::printf(CONSOLE_HEADER "m_toolTip = CreateWindowEx() failed");

				::DeleteFont(m_uiHfont);
				m_uiHfont = NULL;

				DestroyWindow(m_combobox);
				m_combobox = NULL;
				return -1;
			}

			CONST TOOLINFO toolInfo = {
				sizeof(toolInfo),
				TTF_IDISHWND | TTF_SUBCLASS,
				parentWnd,
				(UINT_PTR) m_combobox,
				{},
				{},
				LPSTR_TEXTCALLBACK,
				{},
				NULL
			};
			uSendMessage(m_toolTip, TTM_ADDTOOL, 0, (LPARAM) &toolInfo);  // Associate the tooltip with the tool.

			// Sets the font that a control is to use when drawing text
			uSendMessage(m_combobox, WM_SETFONT, (WPARAM) m_uiHfont, MAKELPARAM(1, 0));

			// get metrics
			RECT rc;
			GetWindowRect(m_combobox, &rc);
			m_HEIGHT = RECT_CY(rc);

			++comboboxParents[parentWnd];

			// get ouput names and add to combo box
			initCombobox();

			return 0;
		}

		case WM_DESTROY:
		{
			//console::printf(CONSOLE_HEADER "got WM_DESTROY");

			--comboboxParents[parentWnd];
			if (comboboxParents[parentWnd] == 0)
				comboboxParents.erase(parentWnd);

			DestroyWindow(m_combobox);
			m_combobox = NULL;

			DestroyWindow(m_toolTip);
			m_toolTip = NULL;

			DeleteFont(m_uiHfont);
			m_uiHfont = NULL;

			return 0;
		}

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO ptr = (LPMINMAXINFO) lp;
			ptr->ptMinTrackSize.x = m_fullSizeWidth + (4 * m_extraMargin);

			ptr->ptMinTrackSize.y = m_HEIGHT;
			ptr->ptMaxTrackSize.y = m_HEIGHT;
			return 0;
		}

		case WM_WINDOWPOSCHANGED:
		{
			//console::printf(CONSOLE_HEADER "got WM_WINDOWPOSCHANGED");

			LPWINDOWPOS ptr = (LPWINDOWPOS) lp;
			if (!(ptr->flags & SWP_NOSIZE))
				SetWindowPos(m_combobox, HWND_TOP, 0, 0, ptr->cx, m_HEIGHT, SWP_NOZORDER);

			return 0;
		}

		case WM_COMMAND:
		{
			switch (HIWORD(wp))
			{
				case CBN_SELCHANGE:
				{
					//console::printf(CONSOLE_HEADER "got CBN_SELCHANGE");

					ComboboxItem item;
					if (!getSelectItem(item))
						break;

					menuExecCommand(item.guid);
					return 0;
				}

				case CBN_DROPDOWN:
				{
					//console::printf(CONSOLE_HEADER "got CBN_DROPDOWN");

					initCombobox();

					if (!setComboboxWidth(m_fullSizeWidth + (2 * m_extraMargin)))
						break;
					return 0;
				}

				default:
					break;
			}

			break;
		}

		case WM_USER_DSP_CORE_CHANGE:
		{
			//console::printf(CONSOLE_HEADER "got WM_USER_DSP_CORE_CHANGE");

			initCombobox();
			return 0;
		}

		case WM_NOTIFY:
		{
			switch (((LPNMHDR) lp)->code)
			{
				case TTN_GETDISPINFO:
				{
					//console::printf(CONSOLE_HEADER "got TTN_GETDISPINFO");

					ComboboxItem item;
					if (!getSelectItem(item))
						break;

					LPNMTTDISPINFO ptr = (LPNMTTDISPINFO) lp;
					MultiByteToWideChar(CP_UTF8, 0, item.name.c_str(), -1, ptr->szText, ARRAY_LENGTH(ptr->szText) - 1);

					return 0;
				}

				default:
					break;
			}

			break;
		}

		default:
		{
			//console::printf(CONSOLE_HEADER "default case: %u", msg);
			break;
		}
	}

	// Calls the default window procedure to provide default processing for any window messages that an application does not process.
	// This function ensures that every message is processed.
	return uDefWindowProc(parentWnd, msg, wp, lp);
}


void DspConfigWatcher::on_core_settings_change(const dsp_chain_config & /*p_newdata*/)
{
	//console::printf(CONSOLE_HEADER "comboboxParents.size(): %d", comboboxParents.size());

	for (const auto &i : comboboxParents)
		uPostMessage(i.first, WM_USER_DSP_CORE_CHANGE, 0, 0);  // no point in blocking
}
