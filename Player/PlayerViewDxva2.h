#pragma once

#include <memory>

struct IFrameListener;

struct IDirect3D9;
struct IDirect3DDevice9;
struct IDirect3DSurface9;
struct IDirectXVideoProcessorService;
struct IDirectXVideoProcessor;

class CPlayerDoc;

// CPlayerViewDxva2 view

class CPlayerViewDxva2 : public CView
{
	friend class FrameListenerDxva2;

	DECLARE_DYNCREATE(CPlayerViewDxva2)

protected:
	CPlayerViewDxva2();           // protected constructor used by dynamic creation
	virtual ~CPlayerViewDxva2();

public:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

	CPlayerDoc* GetDocument() const;

	void updateFrame();

protected:
	DECLARE_MESSAGE_MAP()
private:
	bool InitializeD3D9();
	bool InitializeDXVA2();
	void DestroyDXVA2();
	void DestroyD3D9();
	bool EnableDwmQueuing();
	bool CreateDXVA2VPDevice(REFGUID guid, bool bDXVA2SW);
	bool ResetDevice(bool resizeSource);
	bool ProcessVideo();

private:
	std::unique_ptr<IFrameListener> m_frameListener;
	CSize m_sourceSize;

	CCriticalSection m_csSurface;

	CComPtr<IDirect3D9> m_pD3D9;
	CComPtr<IDirect3DDevice9>  m_pD3DD9;
	CComPtr<IDirect3DSurface9> m_pD3DRT;
	CComPtr<IDirectXVideoProcessorService> m_pDXVAVPS;
	CComPtr<IDirect3DSurface9> m_pMainStream;
	CComPtr<IDirectXVideoProcessor> m_pDXVAVPD;

	BOOL m_bDwmQueuing;

	LONG m_ProcAmpValues[4];
	LONG m_NFilterValues[6];
	LONG m_DFilterValues[6];

public:
	afx_msg void OnPaint();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
