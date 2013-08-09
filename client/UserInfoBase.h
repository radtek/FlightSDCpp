/*
* Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef USERINFOBASE_H
#define USERINFOBASE_H

#include "forward.h"

//#include "AdcHub.h"
//#include "OnlineUser.h"
#include "User.h"
#include "Util.h"

namespace dcpp
{

class UserInfoBase
{
    public:
        UserInfoBase() { }
        
        void getList(const string& hubHint);
        void browseList(const string& hubHint);
        void checkList(const string& hubHint);
        void doReport(const string& hubHint);
        void matchQueue(const string& hubHint);
        void pm(const string& hubHint);
        void grant(const string& hubHint);
        void grantHour(const string& hubHint);
        void grantDay(const string& hubHint);
        void grantWeek(const string& hubHint);
        void ungrant();
        void addFav();
        void removeAll();
        void connectFav();
        
        virtual const UserPtr& getUser() const = 0;
        
        static uint8_t getImage(const Identity& identity, const Client* c);
};

} // namespace dcpp

#endif