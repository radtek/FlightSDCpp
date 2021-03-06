/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"

#include "Resource.h"

#include "DirectoryListingFrm.h"
#include "WinUtil.h"
#include "LineDlg.h"
#include "PrivateFrame.h"

#include "../client/File.h"
#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ADLSearch.h"
#include "../client/MerkleTree.h"
#include "../client/User.h"
#include "../client/ClientManager.h"

DirectoryListingFrame::FrameMap DirectoryListingFrame::frames;
int DirectoryListingFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_TYPE, COLUMN_EXACTSIZE, COLUMN_SIZE, COLUMN_TTH,
                                               COLUMN_PATH,
                                               COLUMN_HIT,
                                               COLUMN_TS,
                                               COLUMN_BITRATE,
                                               COLUMN_MEDIA_XY, COLUMN_MEDIA_VIDEO, COLUMN_MEDIA_AUDIO
                                             };
int DirectoryListingFrame::columnSizes[] = { 300, 60, 100, 100, 200, 300, 30, 100, 50, 100, 100, 100  };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::TYPE, ResourceManager::EXACT_SIZE, ResourceManager::SIZE, ResourceManager::TTH_ROOT,
                                                  ResourceManager::PATH, ResourceManager::DOWNLOADED,
                                                  ResourceManager::ADDED, ResourceManager::BITRATE, ResourceManager::MEDIA_X_Y,
                                                  ResourceManager::MEDIA_VIDEO, ResourceManager::MEDIA_AUDIO
                                                };

DirectoryListingFrame::UserMap DirectoryListingFrame::lists;

void DirectoryListingFrame::openWindow(const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed)
{
    UserIter i = lists.find(aUser);
    if (i != lists.end())
    {
        if (!BOOLSETTING(POPUNDER_FILELIST))
        {
            i->second->speed = aSpeed;
            i->second->MDIActivate(i->second->m_hWnd);
        }
    }
    else
    {
        HWND aHWND = NULL;
        DirectoryListingFrame* frame = new DirectoryListingFrame(aUser, aSpeed);
        if (BOOLSETTING(POPUNDER_FILELIST))
        {
            aHWND = WinUtil::hiddenCreateEx(frame);
        }
        else
        {
            aHWND = frame->CreateEx(WinUtil::mdiClient);
        }
        if (aHWND != 0)
        {
            frame->loadFile(aFile, aDir);
            frames.insert(FramePair(frame->m_hWnd, frame));
        }
        else
        {
            delete frame;
        }
    }
}

void DirectoryListingFrame::openWindow(const HintedUser& aUser, const string& txt, int64_t aSpeed)
{
    UserIter i = lists.find(aUser);
    if (i != lists.end())
    {
        i->second->speed = aSpeed;
        i->second->loadXML(txt);
    }
    else
    {
        DirectoryListingFrame* frame = new DirectoryListingFrame(aUser, aSpeed);
        if (BOOLSETTING(POPUNDER_FILELIST))
        {
            WinUtil::hiddenCreateEx(frame);
        }
        else
        {
            frame->CreateEx(WinUtil::mdiClient);
        }
        frame->loadXML(txt);
        frames.insert(FramePair(frame->m_hWnd, frame));
    }
}

DirectoryListingFrame::DirectoryListingFrame(const HintedUser& aUser, int64_t aSpeed) :
    statusContainer(STATUSCLASSNAME, this, STATUS_MESSAGE_MAP), treeContainer(WC_TREEVIEW, this, CONTROL_MESSAGE_MAP),
    listContainer(WC_LISTVIEW, this, CONTROL_MESSAGE_MAP), historyIndex(0), loading(true),
    treeRoot(NULL), skipHits(0), files(0), speed(aSpeed), updating(false), dl(new DirectoryListing(aUser)), searching(false)
{
    lists.insert(make_pair(aUser, this));
}

void DirectoryListingFrame::loadFile(const tstring& name, const tstring& dir)
{
    m_FL_LoadSec = GET_TICK();
    ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
    //don't worry about cleanup, the object will delete itself once the thread has finished it's job
    ThreadedDirectoryListing* tdl = new ThreadedDirectoryListing(this, Text::fromT(name), Util::emptyString, dir);
    loading = true;
    tdl->start();
}

void DirectoryListingFrame::loadXML(const string& txt)
{
    m_FL_LoadSec = GET_TICK();
    ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
    //don't worry about cleanup, the object will delete itself once the thread has finished it's job
    ThreadedDirectoryListing* tdl = new ThreadedDirectoryListing(this, Util::emptyString, txt);
    loading = true;
    tdl->start();
}

LRESULT DirectoryListingFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{


    CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
    ctrlStatus.Attach(m_hWndStatusBar);
    ctrlStatus.SetFont(WinUtil::boldFont);
    statusContainer.SubclassWindow(ctrlStatus.m_hWnd);
    
    ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT, WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
    
    if (BOOLSETTING(USE_EXPLORER_THEME) &&
            ((WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1) //WinXP & WinSvr2003
             || (WinUtil::getOsMajor() >= 6))) //Vista & Win7
    {
        SetWindowTheme(ctrlTree.m_hWnd, L"explorer", NULL);
    }
    
    treeContainer.SubclassWindow(ctrlTree);
    ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FILES);
    listContainer.SubclassWindow(ctrlList);
    ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
    
    ctrlList.SetBkColor(WinUtil::bgColor);
    ctrlList.SetTextBkColor(WinUtil::bgColor);
    ctrlList.SetTextColor(WinUtil::textColor);
    
    
    ctrlTree.SetBkColor(WinUtil::bgColor);
    ctrlTree.SetTextColor(WinUtil::textColor);
    
    WinUtil::splitTokens(columnIndexes, SETTING(DIRECTORYLISTINGFRAME_ORDER), COLUMN_LAST);
    WinUtil::splitTokens(columnSizes, SETTING(DIRECTORYLISTINGFRAME_WIDTHS), COLUMN_LAST);
    for (uint8_t j = 0; j < COLUMN_LAST; j++)
    {
        int fmt = ((j == COLUMN_SIZE) || (j == COLUMN_EXACTSIZE) || (j == COLUMN_TYPE)) ? LVCFMT_RIGHT : LVCFMT_LEFT;
        ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
    }
    ctrlList.setColumnOrderArray(COLUMN_LAST, columnIndexes);
    ctrlList.setVisible(SETTING(DIRECTORYLISTINGFRAME_VISIBLE));
    
    ctrlTree.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
    ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
    ctrlList.setSortColumn(COLUMN_FILENAME);
    
    ctrlFind.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                    BS_PUSHBUTTON, 0, IDC_FIND);
    ctrlFind.SetWindowText(CTSTRING(FIND));
    ctrlFind.SetFont(WinUtil::systemFont);
    
    ctrlFindNext.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                        BS_PUSHBUTTON, 0, IDC_NEXT);
    ctrlFindNext.SetWindowText(CTSTRING(NEXT));
    ctrlFindNext.SetFont(WinUtil::systemFont);
    
    ctrlMatchQueue.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                          BS_PUSHBUTTON, 0, IDC_MATCH_QUEUE);
    ctrlMatchQueue.SetWindowText(CTSTRING(MATCH_QUEUE));
    ctrlMatchQueue.SetFont(WinUtil::systemFont);
    
    ctrlListDiff.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                        BS_PUSHBUTTON, 0, IDC_FILELIST_DIFF);
    ctrlListDiff.SetWindowText(CTSTRING(FILE_LIST_DIFF));
    ctrlListDiff.SetFont(WinUtil::systemFont);
    
    SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
    SetSplitterPanes(ctrlTree.m_hWnd, ctrlList.m_hWnd);
    m_nProportionalPos = 2500;
    string nick = ClientManager::getInstance()->getNicks(dl->getHintedUser())[0];
    treeRoot = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, Text::toT(nick).c_str(), WinUtil::getDirIconIndex(), WinUtil::getDirIconIndex(), 0, 0, (LPARAM)dl->getRoot(), NULL, TVI_SORT);
    dcassert(treeRoot != NULL);
    
    memzero(statusSizes, sizeof(statusSizes));
    statusSizes[STATUS_FILE_LIST_DIFF] = WinUtil::getTextWidth(TSTRING(FILE_LIST_DIFF), m_hWnd) + 8;
    statusSizes[STATUS_MATCH_QUEUE] = WinUtil::getTextWidth(TSTRING(MATCH_QUEUE), m_hWnd) + 8;
    statusSizes[STATUS_FIND] = WinUtil::getTextWidth(TSTRING(FIND), m_hWnd) + 8;
    statusSizes[STATUS_NEXT] = WinUtil::getTextWidth(TSTRING(NEXT), m_hWnd) + 8;
    
    ctrlStatus.SetParts(STATUS_LAST, statusSizes);
    
    ctrlTree.EnableWindow(FALSE);
    
    SettingsManager::getInstance()->addListener(this);
    closed = false;
    
    setWindowTitle();
    
    bHandled = FALSE;
    return 1;
}

void DirectoryListingFrame::updateTree(DirectoryListing::Directory* aTree, HTREEITEM aParent)
{
    for (DirectoryListing::Directory::Iter i = aTree->directories.begin(); i != aTree->directories.end(); ++i)
    {
        if (!loading)
        {
            throw AbortException();
        }
        
        tstring name = Text::toT((*i)->getName());
        int index = (*i)->getComplete() ? WinUtil::getDirIconIndex() : WinUtil::getDirMaskedIndex();
        HTREEITEM ht = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, name.c_str(), index, index, 0, 0, (LPARAM) * i, aParent, TVI_SORT);
        if ((*i)->getAdls())
            ctrlTree.SetItemState(ht, TVIS_BOLD, TVIS_BOLD);
        updateTree(*i, ht);
    }
}

void DirectoryListingFrame::refreshTree(const tstring& root)
{
    if (!loading)
    {
        throw AbortException();
    }
    
    ctrlTree.SetRedraw(FALSE);
    
    HTREEITEM ht = findItem(treeRoot, root);
    if (ht == NULL)
    {
        ht = treeRoot;
    }
    
    if (DirectoryListing::Directory* d = (DirectoryListing::Directory*)ctrlTree.GetItemData(ht))
    {
        HTREEITEM next = NULL;
        while ((next = ctrlTree.GetChildItem(ht)) != NULL)
        {
            ctrlTree.DeleteItem(next);
        }
        
        updateTree(d, ht);
        
        ctrlTree.Expand(treeRoot);
        
        int index = d->getComplete() ? WinUtil::getDirIconIndex() : WinUtil::getDirMaskedIndex();
        ctrlTree.SetItemImage(ht, index, index);
        
        ctrlTree.SelectItem(NULL);
        selectItem(root);
        
    }
    ctrlTree.SetRedraw(TRUE);
}

void DirectoryListingFrame::updateStatus()
{
    if (!searching && !updating && ctrlStatus.IsWindow())
    {
        int cnt = ctrlList.GetSelectedCount();
        int64_t total = 0;
        if (cnt == 0)
        {
            cnt = ctrlList.GetItemCount();
            total = ctrlList.forEachT(ItemInfo::TotalSize()).total;
        }
        else
        {
            total = ctrlList.forEachSelectedT(ItemInfo::TotalSize()).total;
        }
        
        tstring tmp = TSTRING(ITEMS) + _T(": ") + Util::toStringW(cnt);
        bool u = false;
        
        int w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
        if (statusSizes[STATUS_SELECTED_FILES] < w)
        {
            statusSizes[STATUS_SELECTED_FILES] = w;
            u = true;
        }
        ctrlStatus.SetText(STATUS_SELECTED_FILES, tmp.c_str());
        
        tmp = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(total);
        w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
        if (statusSizes[STATUS_SELECTED_SIZE] < w)
        {
            statusSizes[STATUS_SELECTED_SIZE] = w;
            u = true;
        }
        ctrlStatus.SetText(STATUS_SELECTED_SIZE, tmp.c_str());
        
        if (u)
            UpdateLayout(TRUE);
    }
}

void DirectoryListingFrame::initStatus()
{
    files = dl->getTotalFileCount();
    size = Util::formatBytes(dl->getTotalSize());
    
    tstring tmp = TSTRING(FILES) + _T(": ") + Util::toStringW(dl->getTotalFileCount(true));
    statusSizes[STATUS_TOTAL_FILES] = WinUtil::getTextWidth(tmp, m_hWnd);
    ctrlStatus.SetText(STATUS_TOTAL_FILES, tmp.c_str());
    
    tmp = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(dl->getTotalSize(true));
    statusSizes[STATUS_TOTAL_SIZE] = WinUtil::getTextWidth(tmp, m_hWnd);
    ctrlStatus.SetText(STATUS_TOTAL_SIZE, tmp.c_str());
    
    tmp = TSTRING(SPEED) + _T(": ") + Util::formatBytesW(speed) + _T("/s");
    statusSizes[STATUS_SPEED] = WinUtil::getTextWidth(tmp, m_hWnd);
    ctrlStatus.SetText(STATUS_SPEED, tmp.c_str());
    
    UpdateLayout(FALSE);
}

LRESULT DirectoryListingFrame::onSelChangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMTREEVIEW* p = (NMTREEVIEW*) pnmh;
    
    if (p->itemNew.state & TVIS_SELECTED)
    {
        DirectoryListing::Directory* d = (DirectoryListing::Directory*)p->itemNew.lParam;
        changeDir(d, TRUE);
        addHistory(dl->getPath(d));
    }
    return 0;
}


void DirectoryListingFrame::addHistory(const string& name)
{
    history.erase(history.begin() + historyIndex, history.end());
    while (history.size() > 25)
        history.pop_front();
    history.push_back(name);
    historyIndex = history.size();
}

void DirectoryListingFrame::changeDir(DirectoryListing::Directory* d, BOOL enableRedraw)
{
    ctrlList.SetRedraw(FALSE);
    updating = true;
    clearList();
    
    for (DirectoryListing::Directory::Iter i = d->directories.begin(); i != d->directories.end(); ++i)
    {
        ctrlList.insertItem(ctrlList.GetItemCount(), new ItemInfo(*i), (*i)->getComplete() ? WinUtil::getDirIconIndex() : WinUtil::getDirMaskedIndex());
    }
    for (auto j = d->files.cbegin(); j != d->files.cend(); ++j)
    {
        const ItemInfo* ii = new ItemInfo(*j);
        ctrlList.insertItem(ctrlList.GetItemCount(), ii, WinUtil::getIconIndex(ii->getText(COLUMN_FILENAME)));
    }
    ctrlList.resort();
    ctrlList.SetRedraw(enableRedraw);
    updating = false;
    updateStatus();
    
    if (!d->getComplete())
    {
        if (dl->getUser()->isOnline())
        {
            try
            {
                // TODO provide hubHint?
                QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_PARTIAL_LIST, dl->getPath(d));
                ctrlStatus.SetText(STATUS_TEXT, CTSTRING(DOWNLOADING_LIST));
            }
            catch (const QueueException& e)
            {
                ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
            }
        }
        else
        {
            ctrlStatus.SetText(STATUS_TEXT, CTSTRING(USER_OFFLINE));
        }
    }
}

void DirectoryListingFrame::up()
{
    HTREEITEM t = ctrlTree.GetSelectedItem();
    if (t == NULL)
        return;
    t = ctrlTree.GetParentItem(t);
    if (t == NULL)
        return;
    ctrlTree.SelectItem(t);
}

void DirectoryListingFrame::back()
{
    if (history.size() > 1 && historyIndex > 1)
    {
        size_t n = min(historyIndex, history.size()) - 1;
        deque<string> tmp = history;
        selectItem(Text::toT(history[n - 1]));
        historyIndex = n;
        history = tmp;
    }
}

void DirectoryListingFrame::forward()
{
    if (history.size() > 1 && historyIndex < history.size())
    {
        size_t n = min(historyIndex, history.size() - 1);
        deque<string> tmp = history;
        selectItem(Text::toT(history[n]));
        historyIndex = n + 1;
        history = tmp;
    }
}

LRESULT DirectoryListingFrame::onDoubleClickFiles(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;
    
    HTREEITEM t = ctrlTree.GetSelectedItem();
    if (t != NULL && item->iItem != -1)
    {
        const ItemInfo* ii = ctrlList.getItemData(item->iItem);
        
        if (ii->type == ItemInfo::FILE)
        {
            if(ShareManager::getInstance()->toRealPath(ii->file->getTTH()) != Util::emptyString){
                PostMessage(WM_COMMAND, IDC_OPEN_FILE);
                return 0;
            }

            try
            {
                dl->download(ii->file, SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(ii->getText(COLUMN_FILENAME)), false, WinUtil::isShift(), QueueItem::DEFAULT);
            }
            catch (const Exception& e)
            {
                ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
            }
        }
        else
        {
            HTREEITEM ht = ctrlTree.GetChildItem(t);
            while (ht != NULL)
            {
                if ((DirectoryListing::Directory*)ctrlTree.GetItemData(ht) == ii->dir)
                {
                    ctrlTree.SelectItem(ht);
                    break;
                }
                ht = ctrlTree.GetNextSiblingItem(ht);
            }
        }
    }
    return 0;
}

LRESULT DirectoryListingFrame::onDownloadDir(WORD , WORD , HWND , BOOL&)
{
    HTREEITEM t = ctrlTree.GetSelectedItem();
    if (t != NULL)
    {
        DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
        try
        {
            dl->download(dir, SETTING(DOWNLOAD_DIRECTORY), WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
        }
        catch (const Exception& e)
        {
            ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
        }
    }
    return 0;
}

LRESULT DirectoryListingFrame::onDownloadDirWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    HTREEITEM t = ctrlTree.GetSelectedItem();
    if (t != NULL)
    {
        DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
        
        QueueItem::Priority p;
        switch (wID - 90)
        {
            case IDC_PRIORITY_PAUSED:
                p = QueueItem::PAUSED;
                break;
            case IDC_PRIORITY_LOWEST:
                p = QueueItem::LOWEST;
                break;
            case IDC_PRIORITY_LOW:
                p = QueueItem::LOW;
                break;
            case IDC_PRIORITY_NORMAL:
                p = QueueItem::NORMAL;
                break;
            case IDC_PRIORITY_HIGH:
                p = QueueItem::HIGH;
                break;
            case IDC_PRIORITY_HIGHEST:
                p = QueueItem::HIGHEST;
                break;
            default:
                p = QueueItem::DEFAULT;
                break;
        }
        
        try
        {
            dl->download(dir, SETTING(DOWNLOAD_DIRECTORY), WinUtil::isShift(), p);
        }
        catch (const Exception& e)
        {
            ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
        }
    }
    return 0;
}

LRESULT DirectoryListingFrame::onDownloadDirTo(WORD , WORD , HWND , BOOL&)
{
    HTREEITEM t = ctrlTree.GetSelectedItem();
    if (t != NULL)
    {
        DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
        tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
        if (WinUtil::browseDirectory(target, m_hWnd))
        {
            WinUtil::addLastDir(target);
            
            try
            {
                dl->download(dir, Text::fromT(target), WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
            }
            catch (const Exception& e)
            {
                ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
            }
        }
    }
    return 0;
}

void DirectoryListingFrame::downloadList(const tstring& aTarget, bool view /* = false */, QueueItem::Priority prio /* = QueueItem::Priority::DEFAULT */)
{
    string target = aTarget.empty() ? SETTING(DOWNLOAD_DIRECTORY) : Text::fromT(aTarget);
    
    int i = -1;
    while ((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1)
    {
        const ItemInfo* ii = ctrlList.getItemData(i);
        
        try
        {
            if (ii->type == ItemInfo::FILE)
            {
                if (view)
                {
                    File::deleteFile(target + Util::validateFileName(ii->file->getName()));
                }
                dl->download(ii->file, target + Text::fromT(ii->getText(COLUMN_FILENAME)), view, WinUtil::isShift() || view, prio);
            }
            else if (!view)
            {
                dl->download(ii->dir, target, WinUtil::isShift(), prio);
            }
        }
        catch (const Exception& e)
        {
            ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
        }
    }
}

LRESULT DirectoryListingFrame::onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if(ctrlList.GetSelectedCount() != 1){
        return 0;
    }

    const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
    if(ii->type == ItemInfo::FILE){
        string fpath = ShareManager::getInstance()->toRealPath(ii->file->getTTH());
        if(fpath != Util::emptyString){
            WinUtil::openFile(Text::toT(fpath));
        }
    }
    return 0;
}

LRESULT DirectoryListingFrame::onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)), false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
    return 0;
}

LRESULT DirectoryListingFrame::onDownloadWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    QueueItem::Priority p;
    
    switch (wID)
    {
        case IDC_PRIORITY_PAUSED:
            p = QueueItem::PAUSED;
            break;
        case IDC_PRIORITY_LOWEST:
            p = QueueItem::LOWEST;
            break;
        case IDC_PRIORITY_LOW:
            p = QueueItem::LOW;
            break;
        case IDC_PRIORITY_NORMAL:
            p = QueueItem::NORMAL;
            break;
        case IDC_PRIORITY_HIGH:
            p = QueueItem::HIGH;
            break;
        case IDC_PRIORITY_HIGHEST:
            p = QueueItem::HIGHEST;
            break;
        default:
            p = QueueItem::DEFAULT;
            break;
    }
    
    downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)), false, p);
    
    return 0;
}

LRESULT DirectoryListingFrame::onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (ctrlList.GetSelectedCount() == 1)
    {
        const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
        
        try
        {
            if (ii->type == ItemInfo::FILE)
            {
                tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + ii->getText(COLUMN_FILENAME);
                if (WinUtil::browseFile(target, m_hWnd))
                {
                    WinUtil::addLastDir(Util::getFilePath(target));
                    dl->download(ii->file, Text::fromT(target), false, WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
                }
            }
            else
            {
                tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
                if (WinUtil::browseDirectory(target, m_hWnd))
                {
                    WinUtil::addLastDir(target);
                    dl->download(ii->dir, Text::fromT(target), WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
                }
            }
        }
        catch (const Exception& e)
        {
            ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
        }
    }
    else
    {
        tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
        if (WinUtil::browseDirectory(target, m_hWnd))
        {
            WinUtil::addLastDir(target);
            downloadList(target, false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
        }
    }
    return 0;
}

LRESULT DirectoryListingFrame::onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    downloadList(Text::toT(Util::getTempPath()), true);
    return 0;
}

LRESULT DirectoryListingFrame::onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    const ItemInfo* ii = ctrlList.getSelectedItem();
    if (ii != NULL && ii->type == ItemInfo::FILE)
    {
        WinUtil::searchHash(ii->file->getTTH());
    }
    return 0;
}


LRESULT DirectoryListingFrame::onMarkAsDownloaded(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int i = -1;
    
    while ((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1)
    {
        ItemInfo* pItemInfo = reinterpret_cast<ItemInfo*>(ctrlList.GetItemData(i));
        
        if (pItemInfo->type == ItemInfo::FILE)
        {
            DirectoryListing::File* pFile = pItemInfo->file;
            
            CFlylinkDBManager::getInstance()->push_download_tth(pFile->getTTH());
            
            pFile->setFlag(DirectoryListing::FLAG_DOWNLOAD);
            ctrlList.updateItem(pItemInfo);
        }
        else
        {
            // Recursively add directory content?
        }
    }
    
    return 0;
}
LRESULT DirectoryListingFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int x = QueueManager::getInstance()->matchListing(*dl);
    
    tstring buf;
    buf.resize(STRING(MATCHED_FILES).length() + 32);
    _sntprintf(&buf[0], buf.size(), CTSTRING(MATCHED_FILES), x);
    ctrlStatus.SetText(STATUS_TEXT, &buf[0]);
    
    return 0;
}

LRESULT DirectoryListingFrame::onListDiff(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    tstring file;
    if (WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getListPath()), _T("File Lists\0*.xml.bz2\0All Files\0*.*\0")))
    {
        DirectoryListing dirList(dl->getHintedUser());
        try
        {
            dirList.loadFile(Text::fromT(file));
            dl->getRoot()->filterList(dirList);
            loading = true;
            refreshTree(Util::emptyStringT);
            loading = false;
            initStatus();
            updateStatus();
        }
        catch (const Exception&)
        {
            /// @todo report to user?
        }
    }
    return 0;
}

LRESULT DirectoryListingFrame::onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (ctrlList.GetSelectedCount() != 1)
        return 0;
        
    tstring fullPath;
    const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
    if (ii->type == ItemInfo::FILE)
    {
        if (!ii->file->getAdls())
            return 0;
        DirectoryListing::Directory* pd = ii->file->getParent();
        while (pd != NULL && pd != dl->getRoot())
        {
            fullPath = Text::toT(pd->getName()) + _T("\\") + fullPath;
            pd = pd->getParent();
        }
    }
    else if (ii->type == ItemInfo::DIRECTORY)
    {
        if (!(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()))
            return 0;
        fullPath = Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath());
    }
    
    selectItem(fullPath);
    
    return 0;
}

HTREEITEM DirectoryListingFrame::findItem(HTREEITEM ht, const tstring& name)
{
    string::size_type i = name.find('\\');
    if (i == string::npos)
        return ht;
        
    for (HTREEITEM child = ctrlTree.GetChildItem(ht); child != NULL; child = ctrlTree.GetNextSiblingItem(child))
    {
        DirectoryListing::Directory* d = (DirectoryListing::Directory*)ctrlTree.GetItemData(child);
        if (Text::toT(d->getName()) == name.substr(0, i))
        {
            return findItem(child, name.substr(i + 1));
        }
    }
    return NULL;
}

void DirectoryListingFrame::selectItem(const tstring& name)
{
    HTREEITEM ht = findItem(treeRoot, name);
    if (ht != NULL)
    {
        ctrlTree.EnsureVisible(ht);
        ctrlTree.SelectItem(ht);
    }
}

LRESULT DirectoryListingFrame::ContextMenu::hookKeyProc(int code, WPARAM wParam, LPARAM lParam)
{
    if(code < 0){
        return CallNextHookEx(hookKey, code, wParam, lParam);
    }

    if(wParam != VK_CONTROL){
        return 0;
    }

    if(code != HC_ACTION){
        return 0;
    }

    //TODO: position items

    // MSDN KeyboardProc callback function :: Bits 31 - The transition state. The value is 0 if the key is being pressed and 1 if it is being released
    // 0x80000000
    if((lParam >> 31) & 1){ //UP
        if(!ContextMenu::options.isQueueInsteadOfDownload){
            return 0;
        }
        ContextMenu::options.isQueueInsteadOfDownload = false;
    }
    else{ //DOWN
        if(ContextMenu::options.isQueueInsteadOfDownload){
            return 0;
        }
        ContextMenu::options.isQueueInsteadOfDownload = true;
    }

    // average on 2000000000x30 iterations:: (lParam >> 31) & 1 or lParam & 0x80000000 ~= 912ms vs bool flag ~= 918ms
    instance.ModifyMenuW(1, MF_STRING | MF_BYPOSITION, (UINT_PTR)instance.GetMenuItemID(1), ((lParam >> 31) & 1)? CTSTRING(DOWNLOAD) : CTSTRING(DOWNLOAD_INTO_QUEUE));
    instance.ModifyMenuW(2, MF_POPUP | MF_BYPOSITION, (UINT_PTR)(HMENU)instance.GetSubMenu(2), ((lParam >> 31) & 1)? CTSTRING(DOWNLOAD_TO) : CTSTRING(DOWNLOAD_INTO_QUEUE_TO));
    return 0;
}

LRESULT DirectoryListingFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        
        if (pt.x == -1 && pt.y == -1)
        {
            WinUtil::getContextMenuPos(ctrlList, pt);
        }
        
        OMenu targetMenu, priorityMenu, copyMenu;
        
        ContextMenu menu;
        targetMenu.CreatePopupMenu();
        priorityMenu.CreatePopupMenu();
        copyMenu.CreatePopupMenu();
        
        targetMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_TO));
        priorityMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_WITH_PRIORITY));
        
        copyMenu.InsertSeparatorFirst(CTSTRING(COPY));
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
        
        menu.instance.AppendMenu(MF_STRING, IDC_DOWNLOAD, CTSTRING(DOWNLOAD));
        menu.instance.AppendMenu(MF_POPUP, (UINT)(HMENU)targetMenu, CTSTRING(DOWNLOAD_TO));
        menu.instance.AppendMenu(MF_POPUP, (UINT)(HMENU)priorityMenu, CTSTRING(DOWNLOAD_WITH_PRIORITY));
        menu.instance.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
        menu.instance.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
        menu.instance.AppendMenu(MF_SEPARATOR);
        menu.instance.AppendMenu(MF_STRING, IDC_MARK_AS_DOWNLOADED, CTSTRING(MARK_AS_DOWNLOADED));
        menu.instance.AppendMenu(MF_SEPARATOR);
        menu.instance.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
        menu.instance.AppendMenu(MF_SEPARATOR);
        menu.instance.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
        menu.instance.SetMenuDefaultItem(IDC_DOWNLOAD);
        
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST, CTSTRING(LOWEST));
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW, CTSTRING(LOW));
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL, CTSTRING(NORMAL));
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH, CTSTRING(HIGH));
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST, CTSTRING(HIGHEST));
        
        int n = 0;
        
        const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
        
        if (ctrlList.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE)
        {
            if(ShareManager::getInstance()->toRealPath(ii->file->getTTH()) != Util::emptyString){
                menu.instance.AppendMenu(MF_STRING, IDC_OPEN_FILE, CTSTRING(OPEN));
                menu.instance.SetMenuDefaultItem(IDC_OPEN_FILE);
            }

            menu.instance.InsertSeparatorFirst(Text::toT(Util::getFileName(ii->file->getName())));
            
            //Append Favorite download dirs
            StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
            if (!spl.empty())
            {
                for (StringPairIter i = spl.begin(); i != spl.end(); ++i)
                {
                    targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, Text::toT(i->second).c_str());
                    n++;
                }
                targetMenu.AppendMenu(MF_SEPARATOR);
            }
            
            n = 0;
            targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CTSTRING(BROWSE));
            targets.clear();
            QueueManager::getInstance()->getTargets(ii->file->getTTH(), targets);
            
            if (!targets.empty())
            {
                targetMenu.AppendMenu(MF_SEPARATOR);
                for (StringIter i = targets.begin(); i != targets.end(); ++i)
                {
                    targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + (++n), Text::toT(*i).c_str());
                }
            }
            if (!WinUtil::lastDirs.empty())
            {
                targetMenu.AppendMenu(MF_SEPARATOR);
                for (TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i)
                {
                    targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + (++n), i->c_str());
                }
            }
            
            if (ii->file->getAdls())
            {
                menu.instance.AppendMenu(MF_STRING, IDC_GO_TO_DIRECTORY, CTSTRING(GO_TO_DIRECTORY));
            }
            menu.instance.EnableMenuItem((UINT)(HMENU)copyMenu, MF_BYCOMMAND | MFS_ENABLED);
            prepareMenu(menu.instance, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubs(dl->getHintedUser().user->getCID(), dl->getHintedUser().hint));
            menu.instance.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
        }
        else
        {
            menu.instance.InsertSeparatorFirst(_T(""));
            menu.instance.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_BYCOMMAND | MFS_DISABLED);
            //Append Favorite download dirs
            StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
            if (!spl.empty())
            {
                for (StringPairIter i = spl.begin(); i != spl.end(); ++i)
                {
                    targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, Text::toT(i->second).c_str());
                    n++;
                }
                targetMenu.AppendMenu(MF_SEPARATOR);
            }
            
            n = 0;
            targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CTSTRING(BROWSE));
            if (!WinUtil::lastDirs.empty())
            {
                targetMenu.AppendMenu(MF_SEPARATOR);
                for (TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i)
                {
                    targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + (++n), i->c_str());
                }
            }
            if (ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot())
            {
                menu.instance.AppendMenu(MF_STRING, IDC_GO_TO_DIRECTORY, CTSTRING(GO_TO_DIRECTORY));
            }
            
            prepareMenu(menu.instance, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubs(dl->getUser()->getCID(), dl->getHintedUser().hint));
            menu.instance.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
        }
        
        return TRUE;
    }
    else if (reinterpret_cast<HWND>(wParam) == ctrlTree && ctrlTree.GetSelectedItem() != NULL)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        
        if (pt.x == -1 && pt.y == -1)
        {
            WinUtil::getContextMenuPos(ctrlTree, pt);
        }
        else
        {
            ctrlTree.ScreenToClient(&pt);
            UINT a = 0;
            HTREEITEM ht = ctrlTree.HitTest(pt, &a);
            if (ht != NULL && ht != ctrlTree.GetSelectedItem())
                ctrlTree.SelectItem(ht);
            ctrlTree.ClientToScreen(&pt);
        }
        
        OMenu targetDirMenu, priorityDirMenu;
        
        ContextMenu menu;
        targetDirMenu.CreatePopupMenu();
        priorityDirMenu.CreatePopupMenu();
        
        targetDirMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_TO));
        priorityDirMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_WITH_PRIORITY));
        
        menu.instance.AppendMenu(MF_STRING, IDC_DOWNLOADDIR, CTSTRING(DOWNLOAD));
        menu.instance.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetDirMenu, CTSTRING(DOWNLOAD_TO));
        menu.instance.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityDirMenu, CTSTRING(DOWNLOAD_WITH_PRIORITY));
        menu.instance.AppendMenu(MF_SEPARATOR);
        menu.instance.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
        
        priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED + 90, CTSTRING(PAUSED));
        priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST + 90, CTSTRING(LOWEST));
        priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW + 90, CTSTRING(LOW));
        priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL + 90, CTSTRING(NORMAL));
        priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH + 90, CTSTRING(HIGH));
        priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST + 90, CTSTRING(HIGHEST));
        
        // Strange, windows doesn't change the selection on right-click... (!)
        
        int n = 0;
        //Append Favorite download dirs
        StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
        if (!spl.empty())
        {
            for (StringPairIter i = spl.begin(); i != spl.end(); ++i)
            {
                targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n, Text::toT(i->second).c_str());
                n++;
            }
            targetDirMenu.AppendMenu(MF_SEPARATOR);
        }
        
        n = 0;
        targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIRTO, CTSTRING(BROWSE));
        
        if (!WinUtil::lastDirs.empty())
        {
            targetDirMenu.AppendMenu(MF_SEPARATOR);
            for (TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i)
            {
                targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET_DIR + (++n), i->c_str());
            }
        }
        
        menu.instance.InsertSeparatorFirst(TSTRING(DIRECTORY));
        menu.instance.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
        
        return TRUE;
    }
    
    bHandled = FALSE;
    return FALSE;
}

LRESULT DirectoryListingFrame::onXButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /* lParam */, BOOL& /* bHandled */)
{
    if (GET_XBUTTON_WPARAM(wParam) & XBUTTON1)
    {
        back();
        return TRUE;
    }
    else if (GET_XBUTTON_WPARAM(wParam) & XBUTTON2)
    {
        forward();
        return TRUE;
    }
    
    return FALSE;
}

LRESULT DirectoryListingFrame::onDownloadTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int newId = wID - IDC_DOWNLOAD_TARGET - 1;
    dcassert(newId >= 0);
    
    if (ctrlList.GetSelectedCount() == 1)
    {
        const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
        
        if (ii->type == ItemInfo::FILE)
        {
            if (newId < (int)targets.size())
            {
                try
                {
                    dl->download(ii->file, targets[newId], false, WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
                }
                catch (const Exception& e)
                {
                    ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
                }
            }
            else
            {
                newId -= (int)targets.size();
                dcassert(newId < (int)WinUtil::lastDirs.size());
                downloadList(WinUtil::lastDirs[newId], false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
            }
        }
        else
        {
            dcassert(newId < (int)WinUtil::lastDirs.size());
            downloadList(WinUtil::lastDirs[newId], false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
        }
    }
    else if (ctrlList.GetSelectedCount() > 1)
    {
        dcassert(newId < (int)WinUtil::lastDirs.size());
        downloadList(WinUtil::lastDirs[newId], false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
    }
    return 0;
}

LRESULT DirectoryListingFrame::onDownloadTargetDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int newId = wID - IDC_DOWNLOAD_TARGET_DIR - 1;
    dcassert(newId >= 0);
    
    HTREEITEM t = ctrlTree.GetSelectedItem();
    if (t != NULL)
    {
        DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
        string target = SETTING(DOWNLOAD_DIRECTORY);
        try
        {
            dcassert(newId < (int)WinUtil::lastDirs.size());
            dl->download(dir, Text::fromT(WinUtil::lastDirs[newId]), WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
        }
        catch (const Exception& e)
        {
            ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
        }
    }
    return 0;
}
LRESULT DirectoryListingFrame::onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int newId = wID - IDC_DOWNLOAD_FAVORITE_DIRS;
    dcassert(newId >= 0);
    StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
    
    if (ctrlList.GetSelectedCount() == 1)
    {
        const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
        
        if (ii->type == ItemInfo::FILE)
        {
            if (newId < (int)targets.size())
            {
                try
                {
                    dl->download(ii->file, targets[newId], false, WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
                }
                catch (const Exception& e)
                {
                    ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
                }
            }
            else
            {
                newId -= (int)targets.size();
                dcassert(newId < (int)spl.size());
                downloadList(Text::toT(spl[newId].first), false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
            }
        }
        else
        {
            dcassert(newId < (int)spl.size());
            downloadList(Text::toT(spl[newId].first), false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
        }
    }
    else if (ctrlList.GetSelectedCount() > 1)
    {
        dcassert(newId < (int)spl.size());
        downloadList(Text::toT(spl[newId].first), false, (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
    }
    return 0;
}

LRESULT DirectoryListingFrame::onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int newId = wID - IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS;
    dcassert(newId >= 0);
    
    HTREEITEM t = ctrlTree.GetSelectedItem();
    if (t != NULL)
    {
        DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
        string target = SETTING(DOWNLOAD_DIRECTORY);
        try
        {
            StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
            dcassert(newId < (int)spl.size());
            dl->download(dir, spl[newId].first, WinUtil::isShift(), (ContextMenu::options.isQueueInsteadOfDownload)? QueueItem::PAUSED : QueueItem::DEFAULT);
        }
        catch (const Exception& e)
        {
            ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
        }
    }
    return 0;
}

LRESULT DirectoryListingFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
    if (kd->wVKey == VK_BACK)
    {
        up();
    }
    else if (kd->wVKey == VK_TAB)
    {
        onTab();
    }
    else if (kd->wVKey == VK_LEFT && WinUtil::isAlt())
    {
        back();
    }
    else if (kd->wVKey == VK_RIGHT && WinUtil::isAlt())
    {
        forward();
    }
    else if (kd->wVKey == VK_RETURN)
    {
        if (ctrlList.GetSelectedCount() == 1)
        {
            const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
            if (ii->type == ItemInfo::DIRECTORY)
            {
                HTREEITEM ht = ctrlTree.GetChildItem(ctrlTree.GetSelectedItem());
                while (ht != NULL)
                {
                    if ((DirectoryListing::Directory*)ctrlTree.GetItemData(ht) == ii->dir)
                    {
                        ctrlTree.SelectItem(ht);
                        break;
                    }
                    ht = ctrlTree.GetNextSiblingItem(ht);
                }
            }
            else
            {
                downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
            }
        }
        else
        {
            downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
        }
    }
    return 0;
}

void DirectoryListingFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
    RECT rect;
    GetClientRect(&rect);
    // position bars and offset their dimensions
    UpdateBarsPosition(rect, bResizeBars);
    
    if (ctrlStatus.IsWindow())
    {
        CRect sr;
        int w[STATUS_LAST];
        ctrlStatus.GetClientRect(sr);
        w[STATUS_DUMMY - 1] = sr.right - 16;
        for (int i = STATUS_DUMMY - 2; i >= 0; --i)
        {
            w[i] = max(w[i + 1] - statusSizes[i + 1], 0);
        }
        
        ctrlStatus.SetParts(STATUS_LAST, w);
        ctrlStatus.GetRect(0, sr);
        
        sr.left = w[STATUS_FILE_LIST_DIFF - 1];
        sr.right = w[STATUS_FILE_LIST_DIFF];
        ctrlListDiff.MoveWindow(sr);
        
        sr.left = w[STATUS_MATCH_QUEUE - 1];
        sr.right = w[STATUS_MATCH_QUEUE];
        ctrlMatchQueue.MoveWindow(sr);
        
        sr.left = w[STATUS_FIND - 1];
        sr.right = w[STATUS_FIND];
        ctrlFind.MoveWindow(sr);
        
        sr.left = w[STATUS_NEXT - 1];
        sr.right = w[STATUS_NEXT];
        ctrlFindNext.MoveWindow(sr);
    }
    
    SetSplitterRect(&rect);
}

HTREEITEM DirectoryListingFrame::findFile(const StringSearch& str, HTREEITEM root,
                                          int &foundFile, int &skipHits)
{
    // Check dir name for match
    DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(root);
    if (str.match(dir->getName()))
    {
        if (skipHits == 0)
        {
            foundFile = -1;
            return root;
        }
        else
            skipHits--;
    }
    
    // Force list pane to contain files of current dir
    changeDir(dir, FALSE);
    
    // Check file names in list pane
    for (int i = 0; i < ctrlList.GetItemCount(); i++)
    {
        const ItemInfo* ii = ctrlList.getItemData(i);
        if (ii->type == ItemInfo::FILE)
        {
            if (str.match(ii->file->getName()))
            {
                if (skipHits == 0)
                {
                    foundFile = i;
                    return root;
                }
                else
                    skipHits--;
            }
        }
    }
    
    dcdebug("looking for directories...\n");
    // Check subdirs recursively
    HTREEITEM item = ctrlTree.GetChildItem(root);
    while (item != NULL)
    {
        HTREEITEM srch = findFile(str, item, foundFile, skipHits);
        if (srch)
            return srch;
        else
            item = ctrlTree.GetNextSiblingItem(item);
    }
    
    return 0;
}

void DirectoryListingFrame::findFile(bool findNext)
{
    if (!findNext)
    {
        // Prompt for substring to find
        LineDlg dlg;
        dlg.title = TSTRING(SEARCH_FOR_FILE);
        dlg.description = TSTRING(ENTER_SEARCH_STRING);
        dlg.line = Util::emptyStringT;
        
        if (dlg.DoModal() != IDOK)
            return;
            
        findStr = Text::fromT(dlg.line);
        skipHits = 0;
    }
    else
    {
        skipHits++;
    }
    
    if (findStr.empty())
        return;
        
    // Do a search
    int foundFile = -1, skipHitsTmp = skipHits;
    HTREEITEM const oldDir = ctrlTree.GetSelectedItem();
    HTREEITEM const foundDir = findFile(StringSearch(findStr), ctrlTree.GetRootItem(), foundFile, skipHitsTmp);
    ctrlTree.SetRedraw(TRUE);
    
    if (foundDir)
    {
        // Highlight the directory tree and list if the parent dir/a matched dir was found
        if (foundFile >= 0)
        {
            // SelectItem won't update the list if SetRedraw was set to FALSE and then
            // to TRUE and the item selected is the same as the last one... workaround:
            if (oldDir == foundDir)
                ctrlTree.SelectItem(NULL);
                
            ctrlTree.SelectItem(foundDir);
        }
        else
        {
            // Got a dir; select its parent directory in the tree if there is one
            HTREEITEM parentItem = ctrlTree.GetParentItem(foundDir);
            if (parentItem)
            {
                // Go to parent file list
                ctrlTree.SelectItem(parentItem);
                
                // Locate the dir in the file list
                DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(foundDir);
                
                foundFile = ctrlList.findItem(Text::toT(dir->getName()), -1, false);
            }
            else
            {
                // If no parent exists, just the dir tree item and skip the list highlighting
                ctrlTree.SelectItem(foundDir);
            }
        }
        
        // Remove prev. selection from file list
        if (ctrlList.GetSelectedCount() > 0)
        {
            for (int i = 0; i < ctrlList.GetItemCount(); i++)
                ctrlList.SetItemState(i, 0, LVIS_SELECTED);
        }
        
        // Highlight and focus the dir/file if possible
        if (foundFile >= 0)
        {
            ctrlList.SetFocus();
            ctrlList.EnsureVisible(foundFile, FALSE);
            ctrlList.SetItemState(foundFile, LVIS_SELECTED | LVIS_FOCUSED, (UINT) - 1);
        }
        else
        {
            ctrlTree.SetFocus();
        }
    }
    else
    {
        ctrlTree.SelectItem(oldDir);
        MessageBox(CTSTRING(NO_MATCHES), CTSTRING(SEARCH_FOR_FILE));
    }
}

void DirectoryListingFrame::runUserCommand(UserCommand& uc)
{
    if (!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
        return;
        
    StringMap ucParams = ucLineParams;
    
    set<UserPtr> nicks;
    
    int sel = -1;
    while ((sel = ctrlList.GetNextItem(sel, LVNI_SELECTED)) != -1)
    {
        const ItemInfo* ii = ctrlList.getItemData(sel);
        if (uc.once())
        {
            if (nicks.find(dl->getUser()) != nicks.end())
                continue;
            nicks.insert(dl->getUser());
        }
        if (!dl->getUser()->isOnline())
            return;
        ucParams["fileTR"] = "NONE";
        if (ii->type == ItemInfo::FILE)
        {
            ucParams["type"] = "File";
            ucParams["fileFN"] = dl->getPath(ii->file) + ii->file->getName();
            ucParams["fileSI"] = Util::toString(ii->file->getSize());
            ucParams["fileSIshort"] = Util::formatBytes(ii->file->getSize());
            ucParams["fileTR"] = ii->file->getTTH().toBase32();
        }
        else
        {
            ucParams["type"] = "Directory";
            ucParams["fileFN"] = dl->getPath(ii->dir) + ii->dir->getName();
            ucParams["fileSI"] = Util::toString(ii->dir->getTotalSize());
            ucParams["fileSIshort"] = Util::formatBytes(ii->dir->getTotalSize());
        }
        
        // compatibility with 0.674 and earlier
        ucParams["file"] = ucParams["fileFN"];
        ucParams["filesize"] = ucParams["fileSI"];
        ucParams["filesizeshort"] = ucParams["fileSIshort"];
        ucParams["tth"] = ucParams["fileTR"];
        
        StringMap tmp = ucParams;
        UserPtr tmpPtr = dl->getUser();
        ClientManager::getInstance()->userCommand(dl->getHintedUser(), uc, tmp, true);
    }
}

void DirectoryListingFrame::closeAll()
{
    for (FrameIter i = frames.begin(); i != frames.end(); ++i)
        i->second->PostMessage(WM_CLOSE, 0, 0);
}

LRESULT DirectoryListingFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    tstring sdata;
    int i = -1;
    
    while ((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1)
    {
        const ItemInfo* ii = (ItemInfo*)ctrlList.getItemData(i);
        tstring sCopy;
        
        switch (wID)
        {
            case IDC_COPY_NICK:
                sCopy = WinUtil::getNicks(dl->getHintedUser());
                break;
            case IDC_COPY_FILENAME:
                sCopy = ii->getText(COLUMN_FILENAME);
                break;
            case IDC_COPY_SIZE:
                sCopy = ii->getText(COLUMN_SIZE);
                break;
            case IDC_COPY_LINK:
                if (ii->type == ItemInfo::FILE)
                    sCopy = WinUtil::getMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize());
                break;
            case IDC_COPY_TTH:
                sCopy = ii->getText(COLUMN_TTH);
                break;
            default:
                dcdebug("DIRECTORYLISTINGFRAME DON'T GO HERE\n");
                return 0;
        }
        
        if (!sCopy.empty())
        {
            if (sdata.empty())
                sdata = sCopy;
            else
                sdata = sdata + _T("\r\n") + sCopy;
        }
    }
    
    if (!sdata.empty())
        WinUtil::setClipboard(sdata);
        
    return S_OK;
}

LRESULT DirectoryListingFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    if (loading)
    {
        //tell the thread to abort and wait until we get a notification
        //that it's done.
        dl->setAbort(true);
        return 0;
    }
    
    if (!closed)
    {
        SettingsManager::getInstance()->removeListener(this);
        ctrlList.SetRedraw(FALSE);
        clearList();
        frames.erase(m_hWnd);
        
        ctrlList.saveHeaderOrder(SettingsManager::DIRECTORYLISTINGFRAME_ORDER, SettingsManager::DIRECTORYLISTINGFRAME_WIDTHS,
                                 SettingsManager::DIRECTORYLISTINGFRAME_VISIBLE);
                                 
        closed = true;
        PostMessage(WM_CLOSE);
        return 0;
    }
    else
    {
        bHandled = FALSE;
        return 0;
    }
}

LRESULT DirectoryListingFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
    
    OMenu tabMenu;
    tabMenu.CreatePopupMenu();
    appendUserItems(tabMenu, Util::emptyString); // TODO: hubhint
    tabMenu.AppendMenu(MF_SEPARATOR);
    tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
    
    tstring nick = Text::toT(ClientManager::getInstance()->getNicks(dl->getHintedUser())[0]);
    
    tabMenu.InsertSeparatorFirst(nick);
    tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
    return TRUE;
}

void DirectoryListingFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept
{
    bool refresh = false;
    if (ctrlList.GetBkColor() != WinUtil::bgColor)
    {
        ctrlList.SetBkColor(WinUtil::bgColor);
        ctrlList.SetTextBkColor(WinUtil::bgColor);
        ctrlTree.SetBkColor(WinUtil::bgColor);
        refresh = true;
    }
    if (ctrlList.GetTextColor() != WinUtil::textColor)
    {
        ctrlList.SetTextColor(WinUtil::textColor);
        ctrlTree.SetTextColor(WinUtil::textColor);
        refresh = true;
    }
    if (refresh == true)
    {
        RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
}

LRESULT DirectoryListingFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    switch (wParam)
    {
        case FINISHED:
            loading = false;
            initStatus();
            ctrlStatus.SetFont(WinUtil::systemFont);
            ctrlStatus.SetText(0, CTSTRING(LOADED_FILE_LIST));
            ctrlStatus.SetText(0, Util::toStringW((GET_TICK() - m_FL_LoadSec) / 1000).c_str());
            ctrlTree.EnableWindow(TRUE);
            
            //notify the user that we've loaded the list
            setDirty();
            break;
        case ABORTED:
            loading = false;
            PostMessage(WM_CLOSE, 0, 0);
            break;
        default:
            dcassert(0);
            break;
    }
    return 0;
}

// !SMT!-UI
void DirectoryListingFrame::getItemColor(const Flags::MaskType flags, COLORREF &fg, COLORREF &bg)
{
    if (flags & DirectoryListing::FLAG_SHARED)
        bg = (flags & DirectoryListing::FLAG_NOT_SHARED) ?
             WinUtil::blendColors(SETTING(DUPE_COLOR), SETTING(BACKGROUND_COLOR)) :
             SETTING(DUPE_COLOR);
    if (flags & DirectoryListing::FLAG_DOWNLOAD)
        bg =  WinUtil::blendColors(SETTING(DUPE_COLOR) / 2, SETTING(BACKGROUND_COLOR));
    else if (flags & DirectoryListing::FLAG_OLD_TTH)
        bg =  WinUtil::blendColors(SETTING(DUPE_COLOR) / 3, SETTING(BACKGROUND_COLOR));
}

// !fulDC!
LRESULT DirectoryListingFrame::onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMLVCUSTOMDRAW* plvcd = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);
    
    if (CDDS_PREPAINT == plvcd->nmcd.dwDrawStage)
        return CDRF_NOTIFYITEMDRAW;
        
    if (CDDS_ITEMPREPAINT == plvcd->nmcd.dwDrawStage)
    {
        Flags::MaskType flags = 0;
        ItemInfo *ii = reinterpret_cast<ItemInfo*>(plvcd->nmcd.lItemlParam);
        if (ii->type == ItemInfo::FILE) flags = ii->file->getFlags();
        else if (ii->type == ItemInfo::DIRECTORY) flags = ii->dir->getFlags();
        getItemColor(flags, plvcd->clrText, plvcd->clrTextBk);
    }
    
    return CDRF_DODEFAULT;
}
// !fulDC!
LRESULT DirectoryListingFrame::onCustomDrawTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMLVCUSTOMDRAW* plvcd = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);
    
    if (CDDS_PREPAINT == plvcd->nmcd.dwDrawStage)
        return CDRF_NOTIFYITEMDRAW;
        
    if ((CDDS_ITEMPREPAINT == plvcd->nmcd.dwDrawStage) && ((plvcd->nmcd.uItemState & CDIS_SELECTED) == 0))
    {
        DirectoryListing::Directory* dir = reinterpret_cast<DirectoryListing::Directory*>(plvcd->nmcd.lItemlParam);
        getItemColor(dir->getFlags(), plvcd->clrText, plvcd->clrTextBk);
    }
    
    return CDRF_DODEFAULT;
}

DirectoryListingFrame::ContextMenuOptions DirectoryListingFrame::ContextMenu::options;
DirectoryListingFrame::ContextMenu* DirectoryListingFrame::ContextMenu::_this;

/**
 * @file
 * $Id: DirectoryListingFrm.cpp 568 2011-07-24 18:28:43Z bigmuscle $
 */
