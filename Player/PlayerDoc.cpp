
// PlayerDoc.cpp : implementation of the CPlayerDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Player.h"
#endif

#include "PlayerDoc.h"
#include "AudioPlayerImpl.h"
#include "AudioPlayerWasapi.h"

#include <propkey.h>
#include <memory>

#include <boost/icl/interval_map.hpp>

#include <fstream>

#include <VersionHelpers.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CPlayerDoc::SubtitlesMap : public boost::icl::interval_map<double, std::string>
{};

// CPlayerDoc

IMPLEMENT_DYNCREATE(CPlayerDoc, CDocument)

BEGIN_MESSAGE_MAP(CPlayerDoc, CDocument)
END_MESSAGE_MAP()


// CPlayerDoc construction/destruction

CPlayerDoc::CPlayerDoc()
    : m_frameDecoder(
    IsWindowsVistaOrGreater()
    ? GetFrameDecoder(std::make_unique<AudioPlayerWasapi>())
    : GetFrameDecoder(std::make_unique<AudioPlayerImpl>()))
{
    m_frameDecoder->setDecoderListener(this);
}

CPlayerDoc::~CPlayerDoc()
{
}

BOOL CPlayerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CPlayerDoc serialization

void CPlayerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CPlayerDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CPlayerDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CPlayerDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CPlayerDoc diagnostics

#ifdef _DEBUG
void CPlayerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPlayerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CPlayerDoc commands


BOOL CPlayerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

    if (m_frameDecoder->openFile(lpszPathName))
    {
        OpenSubRipFile(lpszPathName);
        m_frameDecoder->play();
    }

	return TRUE;
}

void CPlayerDoc::OnCloseDocument()
{
	m_frameDecoder->close();
    m_subtitles.reset();
	CDocument::OnCloseDocument();
}


void CPlayerDoc::changedFramePosition(long long frame, long long total)
{
	__raise framePositionChanged(frame, total);
    const double currentTime = m_frameDecoder->getDurationSecs(frame);
    m_currentTime = currentTime;
	__raise totalTimeUpdated(m_frameDecoder->getDurationSecs(total));
    __raise currentTimeUpdated(currentTime);
}

bool CPlayerDoc::pauseResume()
{
	return m_frameDecoder->pauseResume();
}

bool CPlayerDoc::seekByPercent(double percent, int64_t totalDuration)
{
	return m_frameDecoder->seekByPercent(percent, totalDuration);
}

void CPlayerDoc::setVolume(double volume)
{
	m_frameDecoder->setVolume(volume);
}

bool CPlayerDoc::isPlaying() const
{
	return m_frameDecoder->isPlaying();
}

bool CPlayerDoc::isPaused() const
{
	return m_frameDecoder->isPaused();
}

double CPlayerDoc::soundVolume() const
{
	return m_frameDecoder->volume();
}

void CPlayerDoc::OpenSubRipFile(LPCTSTR lpszVideoPathName)
{
    m_subtitles.reset();

    CString subRipPathName(lpszVideoPathName);
    PathRemoveExtension(subRipPathName.GetBuffer());
    subRipPathName.ReleaseBuffer();
    subRipPathName += _T(".srt");

    std::ifstream s(subRipPathName);
    if (!s)
        return;

    auto map(std::make_unique<SubtitlesMap>());

    std::string buffer;
    for (;;)
    {
        if (!std::getline(s, buffer))
            break;
        if (!std::getline(s, buffer))
            break;

        int startHr, startMin, startSec, startMsec;
        int endHr, endMin, endSec, endMsec;

        if (sscanf(
            buffer.c_str(),
            "%d:%d:%d,%d --> %d:%d:%d,%d",
            &startHr, &startMin, &startSec, &startMsec,
            &endHr, &endMin, &endSec, &endMsec) != 8)
        {
            break;
        }

        const double start = startHr * 3600 + startMin * 60 + startSec + startMsec / 1000.;
        const double end = endHr * 3600 + endMin * 60 + endSec + endMsec / 1000.;

        std::string subtitle;
        while (std::getline(s, buffer) && !buffer.empty())
        {
            if (!subtitle.empty())
                subtitle += '\n';
            subtitle += buffer;
        }

        if (!subtitle.empty())
        {
            map->add(std::make_pair(boost::icl::interval<double>::closed(start, end), subtitle));
        }
    }

    if (!map->empty())
    {
        m_subtitles = std::move(map);
    }
}

std::string CPlayerDoc::getSubtitle()
{
    std::string result;
    if (m_subtitles)
    {
        auto it = m_subtitles->find(m_currentTime); 
        if (it != m_subtitles->end())
        {
            result = it->second;
        }
    }
    return result;
}
