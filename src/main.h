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

#pragma once

#include <map>

#define PLUGIN_NAME "DSP Switcher"
#define PLUGIN_VERSION "1.1.0"


class DspSwitcher : private ui_extension::container_ui_extension
{
	private:
	struct ComboboxItem
	{
		pfc::string8 name;
		GUID guid;
		bool selected;

		bool operator== (const ComboboxItem &other) const
		{
			return ((this->selected == other.selected) && (this->guid == other.guid) && (this->name == other.name));
		}

		bool operator!= (const ComboboxItem &other) const
		{
			return !(*this == other);
		}
	};

	typedef pfc::list_t<ComboboxItem> ItemList;


	public:
	// ui_extension::extension_base
	const GUID & get_extension_guid() const override
	{
		static const GUID guid = {0x5eb036ef, 0x9ffd, 0x49ef, {0xbc, 0xb1, 0x6a, 0xdc, 0x27, 0x65, 0x5, 0xf2}};
		return guid;
	}

	void get_name(pfc::string_base & out) const override
	{
		out = PLUGIN_NAME;
	}


	// ui_extension::window
	void get_category(pfc::string_base & out) const override
	{
		out = "Toolbars";
	}

	bool get_description(pfc::string_base & out) const override
	{
		out = "A dropdown combobox for changing DSP presets quickly";
		return true;
	};

	unsigned get_type() const override
	{
		return ui_extension::window_type_t::type_toolbar;
	};


	// ui_helpers::container_window
	class_data & get_class_data()const override
	{
		static const TCHAR className[] = _T("{5EB036EF-9FFD-49EF-BCB1-6ADC276505F2}");
		__implement_get_class_data(className, false);
	}

	LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) override;


	private:
	void initCombobox();
	void clearCombobox() const;
	bool addToCombobox(const char *str) const;

	bool selectItem(const int idx) const;
	bool getSelectItem(ComboboxItem &out) const;

	bool setComboboxWidth(const int width) const;

	ItemList menuFindCommands() const;
	static void menuExecCommand(const GUID &commandGuid);

	ItemList m_comboboxEntries = {};

	HWND m_combobox = NULL;
	HWND m_toolTip = NULL;
	HFONT m_uiHfont = NULL;

	LONG m_HEIGHT = 0;
	int m_minSizeWidth = INT_MAX;
	int m_fullSizeWidth = 0;
};
static ui_extension::window_factory<DspSwitcher> dspSwitcherImpl;


class DspConfigWatcher : public dsp_config_callback
{
	private:
	void on_core_settings_change(const dsp_chain_config &p_newdata) override;
};
static service_factory_single_t<DspConfigWatcher> dspConfigWatcherImpl;

static std::map<HWND, int> comboboxParents;  // <parent HWND, reference count>
