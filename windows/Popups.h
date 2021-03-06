/*
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef Popups_H
#define Popups_H

#pragma once

#include "PropPage.h"
#include "ExListViewCtrl.h"


class Popups : public CPropertyPage<IDD_POPUPS>, public PropPage
{
    public:
        Popups(SettingsManager *s) : PropPage(s)
        {
            title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(BALLOON_POPUPS)).c_str());
            SetTitle(title);
            m_psp.dwFlags |= PSP_RTLREADING;
        };
        
        ~Popups()
        {
            ctrlPopupType.Detach();
            free(title);
        };
        
        BEGIN_MSG_MAP(Sounds)
        MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
        COMMAND_HANDLER(IDC_PREVIEW, BN_CLICKED, onPreview)
        END_MSG_MAP()
        
        LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
        LRESULT onPreview(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        
        // Common PropPage interface
        PROPSHEETPAGE *getPSP()
        {
            return (PROPSHEETPAGE *) * this;
        }
        void write();
        
    protected:
        static ListItem listItems[];
        static Item items[];
        static TextItem texts[];
        TCHAR* title;
        
        ExListViewCtrl ctrlPopups;
        CComboBox ctrlPopupType;
};

#endif //Popups_H

/**
 * @file
 * $Id: Popups.h 308 2007-07-13 18:57:02Z bigmuscle $
 */

