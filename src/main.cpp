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

#include "stdafx.h"

#include "main.h"

#define PLUGIN_NAME "DSP Switcher"
#define PLUGIN_VERSION "1.0.0"
DECLARE_COMPONENT_VERSION
(
	PLUGIN_NAME , PLUGIN_VERSION ,

	PLUGIN_NAME"\n"
	"Compiled on: "__DATE__"\n"
	"https://github.com/Chocobo1/foo_uie_dsp_switcher\n"
	"This plugin is released under BSD 3-Clause license\n"
	"\n"
	"Mike Tzou (Chocobo1)\n"
);


#define WM_USER_DSP_CORE_CHANGE	(WM_USER + 0)
#define WM_USER_SYNC_CHANGE		(WM_USER + 1)


static cfg_int CFG_IDX( { 0xda46969b , 0x448 , 0x474a , { 0x99 , 0x93 , 0x7d , 0x8 , 0xcc , 0xb1 , 0x45 , 0xc7 } } , -1 );


void dsp_preset_switcher::initEntries( pfc::list_t<pfc::string8> &out ) const
{
	// clear existing entries
	addEntry( "" );  // ensure a 1-item height empty item-list is displayed
	clearEntires();

	// get dsp names
	out.remove_all();
	findDspNames( out );

	// add to combo box
	for( t_size i = 0 , imax = out.get_count(); i < imax ; ++i )
	{
		addEntry( out[i] );
	}

	return;
}


bool dsp_preset_switcher::addEntry( const char *str ) const
{
	CONST LRESULT ret = uSendMessageText( wnd_my_combo_box , CB_ADDSTRING , NULL , str );
	if( ret < 0 )
	{
		console::printf( CONSOLE_HEADER "%s() failed" , __FUNCTION__ );
		return false;
	}
	return true;
}


bool dsp_preset_switcher::setEntry( const int idx ) const
{
	CONST LRESULT ret = uSendMessage( wnd_my_combo_box , CB_SETCURSEL , idx , NULL );
	if( ret < 0 )
	{
		console::printf( CONSOLE_HEADER "%s() failed" , __FUNCTION__ );
		return false;
	}
	return true;
}


void dsp_preset_switcher::clearEntires() const
{
	CONST LRESULT ret = uSendMessage( wnd_my_combo_box , CB_RESETCONTENT , 0 , 0 );
	if( ret < 0 )
	{
		console::printf( CONSOLE_HEADER "%s() failed" , __FUNCTION__ );
	}
	return;
}


bool dsp_preset_switcher::getSelection( int &idx_out , pfc::string8 &str_out ) const
{
	idx_out = uSendMessage( wnd_my_combo_box , CB_GETCURSEL , 0 , 0 );
	if( idx_out == CB_ERR )
	{
		console::printf( CONSOLE_HEADER "CB_GETCURSEL get CB_ERR" );
		return false;
	}

	CONST BOOL ret = uComboBox_GetText( wnd_my_combo_box , idx_out , str_out );
	if( !ret )
	{
		console::printf( CONSOLE_HEADER "uComboBox_GetText() failed" );
		return false;
	}

	return true;
}


void dsp_preset_switcher::syncSelection( const int idx ) const
{
	for( t_size i = 0 , imax = dsp_preset_switcher::PARENT_HWND_LIST.get_count(); i < imax ; ++i )
	{
		uPostMessage( dsp_preset_switcher::PARENT_HWND_LIST[i] , WM_USER_SYNC_CHANGE , idx , 0 );
	}
	return;
}


void dsp_preset_switcher::findDspNames( pfc::list_t<pfc::string8> &out ) const
{
	// storing menu handles may not be a good idea, since the user can change DSP preset settings without notify this component

	out.remove_all();

	// enumerate mainmenu items
	service_enum_t<mainmenu_commands> e;
	service_ptr_t<mainmenu_commands_v2> ptr;
	while( e.next( ptr ) )
	{
		for( t_uint32 i = 0 , imax = ptr->get_command_count(); i < imax; ++i )
		{
			// lock-on on DSP settings
			pfc::string8 group_name;
			ptr->get_name( i , group_name );
			const char *DSP_PARENT_STR = "DSP";  // partial match, hope to work with non-English locale
			if( strstr( group_name.toString() , DSP_PARENT_STR ) == nullptr )
				continue;

			// should be a dynamic item
			if( !ptr->is_command_dynamic( i ) )
			{
				console::printf( CONSOLE_HEADER "%s(): item is NOT dynamic!!" , __FUNCTION__ );
				continue;
			}
			const mainmenu_node::ptr dsp_group_node = ptr->dynamic_instantiate( i );

			// should be a group node
			if( dsp_group_node->get_type() != mainmenu_node::type_group )
			{
				console::printf( CONSOLE_HEADER "%s(): node is NOT type_group!!" , __FUNCTION__ );
				continue;
			}

			// enumerate dsp names
			for( t_size j = 0 , jmax = dsp_group_node->get_children_count(); ( j < jmax ) && ( jmax > 1 ) ; ++j )  // jmax == 1 when there only exist "Preferences" items
			{
				const mainmenu_node::ptr dsp_item_node = dsp_group_node->get_child( j );
				if( dsp_item_node->get_type() == mainmenu_node::type_command )
				{
					pfc::string8 n;
					t_uint32 d;
					dsp_item_node->get_display( n , d );
					out.add_item( n );
					//console::printf( CONSOLE_HEADER "%s" , n.toString() );
				}
				else if( dsp_item_node->get_type() == mainmenu_node::type_separator )
				{
					// stop when encountered type_separator
					break;
				}
			}
			return;
		}
	}

	return;
}


bool dsp_preset_switcher::selDspName( const int idx , const char *name ) const
{
	// notify first
	syncSelection( idx );

	// find menu item
	service_enum_t<mainmenu_commands> e;
	service_ptr_t<mainmenu_commands_v2> ptr;
	while( e.next( ptr ) )
	{
		for( t_uint32 i = 0 , imax = ptr->get_command_count(); i < imax; ++i )
		{
			// lock-on on DSP settings
			pfc::string8 group_name;
			ptr->get_name( i , group_name );
			const char *DSP_PARENT_STR = "DSP";  // partial match, hope to work with non-English locale
			if( strstr( group_name.toString() , DSP_PARENT_STR ) == nullptr )
				continue;

			// should be a dynamic item
			if( !ptr->is_command_dynamic( i ) )
			{
				console::printf( CONSOLE_HEADER "%s(): item is NOT dynamic!!" , __FUNCTION__ );
				continue;
			}
			const mainmenu_node::ptr dsp_group_node = ptr->dynamic_instantiate( i );

			// should be a group node
			if( dsp_group_node->get_type() != mainmenu_node::type_group )
			{
				console::printf( CONSOLE_HEADER "%s(): node is NOT type_group!!" , __FUNCTION__ );
				continue;
			}

			// enumerate dsp names
			for( t_size j = 0 , jmax = dsp_group_node->get_children_count(); ( j < jmax ) && ( jmax > 1 ) ; ++j )  // jmax == 1 when there only exist "Preferences" items
			{
				const mainmenu_node::ptr dsp_item_node = dsp_group_node->get_child( j );
				if( dsp_item_node->get_type() == mainmenu_node::type_command )
				{
					pfc::string8 n;
					t_uint32 d;
					dsp_item_node->get_display( n , d );
					if( strncmp( name , n.get_ptr() , strlen( name ) ) == 0 )
					{
						// change core settings
						dsp_item_node->execute( nullptr );
						return true;
					}
				}
			}
			return false;
		}
	}

	return false;
}


LRESULT dsp_preset_switcher::on_message( HWND parent_wnd , UINT msg , WPARAM wp , LPARAM lp )
{
	switch( msg )
	{
		case WM_CREATE:
		{
			if( wnd_my_combo_box != NULL )
			{
				console::printf( CONSOLE_HEADER "Error: wnd_my_combo_box != NULL" );
				return -1;
			}
			if( ui_hfont != NULL )
			{
				console::printf( CONSOLE_HEADER "Error: ui_hfont != NULL" );
				return -1;
			}

			// get columns UI font
			ui_hfont = uCreateIconFont();
			if( ui_hfont == NULL )
			{
				console::printf( CONSOLE_HEADER "uCreateIconFont() failed" );
				return -1;
			}

			wnd_my_combo_box = CreateWindowEx( 0 , WC_COMBOBOX , nullptr , ( CBS_DROPDOWNLIST | CBS_SORT | WS_CHILD | WS_VISIBLE | WS_TABSTOP ) , 0 , 0 , INIT_WIDTH , CW_USEDEFAULT , parent_wnd , NULL , core_api::get_my_instance() , nullptr );
			if( wnd_my_combo_box == NULL )
			{
				console::printf( CONSOLE_HEADER "CreateWindowEx() failed" );

				DeleteFont( ui_hfont );
				ui_hfont = NULL;
				return -1;
			}

			dsp_preset_switcher::PARENT_HWND_LIST.remove_item( parent_wnd );
			dsp_preset_switcher::PARENT_HWND_LIST.add_item( parent_wnd );

			// Sends the specified message to a window or windows. The SendMessage function calls the window procedure for the specified window and does not return until the window procedure has processed the message.
			// Sets the font that a control is to use when drawing text.
			uSendMessage( wnd_my_combo_box , WM_SETFONT , ( WPARAM ) ui_hfont , MAKELPARAM( 1 , 0 ) );

			// get metrics
			RECT rc;
			GetWindowRect( wnd_my_combo_box , &rc );
			HEIGHT = RECT_CY( rc );

			// get dsp names and add to combo box
			pfc::list_t<pfc::string8> l;
			initEntries( l );

			// set initial selection
			const bool ret = setEntry( CFG_IDX );
			if( !ret )
				CFG_IDX = -1;

			// determine width
			// MSDN Remarks: After an application has finished drawing with the new font object , it should always replace a new font object with the original font object.
			CONST HDC dc = GetDC( wnd_my_combo_box );
			CONST HFONT font_old = SelectFont( dc , ui_hfont );
			for( t_size i = 0 , imax = l.get_count(); i < imax ; ++i )
			{
				const char *str = l[i].get_ptr();
				const int cx = ui_helpers::get_text_width( dc , str , strlen( str ) );
				min_width = max( min_width , ( t_size ) cx );
			}
			SelectFont( dc , font_old );
			ReleaseDC( parent_wnd , dc );

			// get min width
			COMBOBOXINFO cbi = { 0 };
			cbi.cbSize = sizeof( cbi );
			GetComboBoxInfo( wnd_my_combo_box , &cbi );

			RECT rc_client;
			GetClientRect( wnd_my_combo_box , &rc_client );
			min_width += RECT_CX( rc_client ) - RECT_CX( cbi.rcItem );

			return 0;
		}

		case WM_DESTROY:
		{
			dsp_preset_switcher::PARENT_HWND_LIST.remove_item( GetParent( wnd_my_combo_box ) );
			DestroyWindow( wnd_my_combo_box );
			wnd_my_combo_box = NULL;

			DeleteFont( ui_hfont );
			ui_hfont = NULL;

			return 0;
		}

		case WM_GETMINMAXINFO:
		{
			CONST LPMINMAXINFO ptr = ( LPMINMAXINFO ) lp;
			ptr->ptMinTrackSize.x = min_width;

			ptr->ptMinTrackSize.y = HEIGHT;
			ptr->ptMaxTrackSize.y = HEIGHT;
			return 0;
		}

		case WM_WINDOWPOSCHANGED:
		{
			CONST LPWINDOWPOS ptr = ( LPWINDOWPOS ) lp;
			if( !( ptr->flags & SWP_NOSIZE ) )
			{
				SetWindowPos( wnd_my_combo_box , HWND_TOP , 0 , 0 , ptr->cx , HEIGHT , SWP_NOZORDER );
			}

			return 0;
		}

		case WM_COMMAND:
		{
			if( wp == ( CBN_SELCHANGE << 16 ) )
			{
				int idx;
				pfc::string8 text;
				bool ret = getSelection( idx , text );
				if( !ret )
					break;

				CFG_IDX = idx;
				ret = selDspName( idx , text.get_ptr() );
				if( !ret )
					syncSelection( -1 );
				return 0;
			}

			break;
		}

		case WM_USER_DSP_CORE_CHANGE:
		{
			if( skip_msg )
			{
				skip_msg = false;
				return 0;
			}

			// just rescan DSP name entries and remove the current selected one
			pfc::list_t<pfc::string8> d;
			initEntries( d );
			CFG_IDX = -1;
			return 0;
		}

		case WM_USER_SYNC_CHANGE:
		{
			if( wp == -1 )
			{
				pfc::list_t<pfc::string8> l;
				initEntries( l );
				return 0;
			}

			skip_msg = true;
			setEntry( wp );
			return 0;
		}

		default:
		{
			//console::printf( CONSOLE_HEADER "default case: %u" , msg );
			break;
		}
	}

	// Calls the default window procedure to provide default processing for any window messages that an application does not process. 
	// This function ensures that every message is processed.
	return uDefWindowProc( parent_wnd , msg , wp , lp );
}


void dspConfigCb::on_core_settings_change( const dsp_chain_config & p_newdata )
{
	CONST DWORD sleep_ms = 250;

	clear_handle_list();
	HANDLE *timer_h = new HANDLE;
	CONST BOOL ret = CreateTimerQueueTimer( timer_h , NULL , ( WAITORTIMERCALLBACK ) dspConfigCb::dsp_core_change_message , NULL , sleep_ms , 0 , WT_EXECUTEONLYONCE );
	if( ret == 0 )
	{
		console::printf( CONSOLE_HEADER "%s(): CreateTimerQueueTimer() failed" , __FUNCTION__ );
		delete timer_h;
		return;
	}
	dspConfigCb::h_list.add_item( timer_h );
	//console::printf( CONSOLE_HEADER "dspConfigCb::h_list.get_count(): %d" , dspConfigCb::h_list.get_count() );
	return;
}


void dspConfigCb::clear_handle_list() const
{
	for( t_size i = 0 , imax = dspConfigCb::h_list.get_count(); i < imax; ++i )
	{
		CONST BOOL ret = DeleteTimerQueueTimer( NULL , *dspConfigCb::h_list[i] , NULL );
		if( ret == 0 )
		{
			console::printf( CONSOLE_HEADER "%s(): DeleteTimerQueueTimer() failed" , __FUNCTION__ );
		}
		delete dspConfigCb::h_list[i];
	}
	dspConfigCb::h_list.remove_all();
	return;
}


VOID CALLBACK dspConfigCb::dsp_core_change_message( PVOID lpParameter , BOOLEAN TimerOrWaitFired )
{
	//console::printf( CONSOLE_HEADER "dsp_preset_switcher::PARENT_HWND_LIST.get_count(): %d" , dsp_preset_switcher::PARENT_HWND_LIST.get_count() );
	for( t_size i = 0 , imax = dsp_preset_switcher::PARENT_HWND_LIST.get_count(); i < imax ; ++i )
	{
		uPostMessage( dsp_preset_switcher::PARENT_HWND_LIST[i] , WM_USER_DSP_CORE_CHANGE , 0 , 0 );  // no point in blocking
	}
	return;
}
