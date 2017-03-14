/*
Copyright (c) 2015, Mike Tzou
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


class dsp_preset_switcher : public ui_extension::container_ui_extension
{
	public:
	// ui_extension::extension_base
	virtual const GUID & get_extension_guid() const
	{
		static const GUID guid = { 0x5eb036ef , 0x9ffd , 0x49ef , { 0xbc , 0xb1 , 0x6a , 0xdc , 0x27 , 0x65 , 0x5 , 0xf2 } };
		return guid;
	}

	virtual void get_name( pfc::string_base & out ) const
	{
		out = "DSP Switcher";
	}


	// ui_extension::window
	virtual void get_category( pfc::string_base & out ) const
	{
		out = "Toolbars";
	}

	virtual bool get_description( pfc::string_base & out ) const
	{
		out = "An dropdown combo box to change DSP presets quickly";
		return true;
	};

	virtual unsigned get_type() const
	{
		return ui_extension::window_type_t::type_toolbar;
	};


	// ui_helpers::container_window
	virtual class_data & get_class_data()const
	{
		__implement_get_class_data( class_name , false );
	}

	virtual LRESULT on_message( HWND wnd , UINT msg , WPARAM wp , LPARAM lp );


	static pfc::list_t<HWND>PARENT_HWND_LIST;


	private:
	void initEntries( pfc::list_t<pfc::string8> &out ) const;
	bool addEntry( const char *str ) const;
	bool setEntry( const int idx ) const;
	void clearEntires() const;

	bool getSelection( int &idx_out , pfc::string8 &str_out ) const;
	void syncSelection( const int idx ) const;

	void findDspNames( pfc::list_t<pfc::string8> &out ) const;
	bool selDspName( const int idx , const char *name ) const;

	const TCHAR *class_name = _T( "{5EB036EF-9FFD-49EF-BCB1-6ADC276505F2}" );
	HWND wnd_my_combo_box = NULL;
	HFONT ui_hfont = NULL;
	bool skip_msg = false;

	const int INIT_WIDTH = 300;
	LONG HEIGHT = 0;
	t_size min_width = 0;
};
pfc::list_t<HWND>dsp_preset_switcher::PARENT_HWND_LIST;
static ui_extension::window_factory<dsp_preset_switcher> dsp_preset_switcher_impl;


class dspConfigCb : public dsp_config_callback
{
	public:
	~dspConfigCb()
	{
		clear_handle_list();
	}

	static pfc::list_t<HANDLE*> h_list;
	virtual void on_core_settings_change( const dsp_chain_config & p_newdata );
	static VOID CALLBACK dsp_core_change_message( PVOID lpParameter , BOOLEAN TimerOrWaitFired );

	private:
	void clear_handle_list() const;
};
pfc::list_t<HANDLE*> dspConfigCb::h_list;
static service_factory_single_t<dspConfigCb> dspConfigCb_impl;
