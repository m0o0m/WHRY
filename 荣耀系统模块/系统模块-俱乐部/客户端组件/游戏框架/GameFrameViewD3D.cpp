#include "StdAfx.h"
#include "GameFrameAvatar.h"
#include "GameFrameViewD3D.h"
#include ".\gameframeviewd3d.h"

//////////////////////////////////////////////////////////////////////////////////
#define IDI_ROLL_TEXT				30									//滚动文字
//渲染消息
#define WM_D3D_RENDER				WM_USER+300							//渲染消息

//////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CGameFrameViewD3D, CGameFrameView)

	//系统消息
	ON_WM_SIZE()
	//ON_WM_PAINT()
	ON_WM_TIMER()

	//自定消息
	ON_MESSAGE(WM_D3D_RENDER, OnMessageD3DRender)

END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////
//函数定义

//绘制数字
VOID DrawNumber(CD3DDevice * pD3DDevice,INT nXPos,INT nYPos,CD3DSprite * pNumberImage,LPCTSTR pszNumber,INT nNumber,LPCTSTR pszFormat,UINT nFormat)
{
	//参数校验
	if(pNumberImage==NULL) return;

	//变量定义
	CString strNumber(pszNumber);
	CSize SizeNumber(pNumberImage->GetWidth()/strNumber.GetLength(),pNumberImage->GetHeight());

	//变量定义	
	TCHAR szValue[32]=TEXT("");
	_sntprintf(szValue,CountArray(szValue),pszFormat,nNumber);

	//调整横坐标
	if((nFormat&DT_LEFT)!=0) nXPos -= 0;
	if((nFormat&DT_CENTER)!=0) nXPos -= (lstrlen(szValue)*SizeNumber.cx)/2;
	if((nFormat&DT_RIGHT)!=0) nXPos -= lstrlen(szValue)*SizeNumber.cx;
	
	//调整纵坐标
	if((nFormat&DT_TOP)!=0) nYPos -= 0;
	if((nFormat&DT_VCENTER)!=0) nYPos -= SizeNumber.cy/2;
	if((nFormat&DT_BOTTOM)!=0) nYPos -= SizeNumber.cy;

	//绘画数字
	INT nIndex=0;
	for(INT i=0; i<lstrlen(szValue);i++)
	{
		nIndex = strNumber.Find(szValue[i]);
		if(nIndex!=-1)
		{
			pNumberImage->DrawImage(pD3DDevice,nXPos,nYPos,SizeNumber.cx,SizeNumber.cy,SizeNumber.cx*nIndex,0,SizeNumber.cx,SizeNumber.cy);
			nXPos += SizeNumber.cx;
		}
	}

	return;
}

//////////////////////////////////////////////////////////////////////////////////

//构造函数
CGameFrameViewD3D::CGameFrameViewD3D()
{
	//状态变量
	m_bInitD3D=false;
	m_hRenderThread=NULL;

	//帧数统计
	m_dwDrawBenchmark=0;
	m_dwDrawLastCount=0;
	m_dwDrawCurrentCount=0L;

	//辅助变量
	m_RectDirtySurface.SetRect(0,0,0,0);
	m_rcText.SetRect(0,0,1,1);

	return;
}

//析构函数
CGameFrameViewD3D::~CGameFrameViewD3D()
{
}

//重置界面
VOID CGameFrameViewD3D::ResetGameView()
{
	return;
}

//调整控件
VOID CGameFrameViewD3D::RectifyControl(INT nWidth, INT nHeight)
{
	return;
}

//界面更新
VOID CGameFrameViewD3D::InvalidGameView(INT nXPos, INT nYPos, INT nWidth, INT nHeight)
{
	//构造位置
	CRect rcInvalid;
	rcInvalid.SetRect(nXPos,nYPos,nXPos+nWidth,nYPos+nHeight);

	//位置调整
	if (rcInvalid.IsRectNull()==TRUE) GetClientRect(&rcInvalid);

	//设置矩形
	if (m_RectDirtySurface.IsRectEmpty()==FALSE)
	{
		//设置变量
		m_RectDirtySurface.top=__min(m_RectDirtySurface.top,rcInvalid.top);
		m_RectDirtySurface.left=__min(m_RectDirtySurface.left,rcInvalid.left);
		m_RectDirtySurface.right=__max(m_RectDirtySurface.right,rcInvalid.right);
		m_RectDirtySurface.bottom=__max(m_RectDirtySurface.bottom,rcInvalid.bottom);
	}
	else
	{
		//设置变量
		m_RectDirtySurface=rcInvalid;
	}

	//刷新窗口
	InvalidateRect(&rcInvalid,FALSE);

	return;
}

//配置设备
VOID CGameFrameViewD3D::OnInitDevice(CD3DDevice * pD3DDevice, INT nWidth, INT nHeight)
{
	//设置变量
	m_bInitD3D=true;

	//获取对象
	ASSERT(CSkinResourceManager::GetInstance()!=NULL);
	CSkinResourceManager * pSkinResourceManager=CSkinResourceManager::GetInstance();

	//获取字体
	LOGFONT LogFont;
	pSkinResourceManager->GetDefaultFont().GetLogFont(&LogFont);

	//创建对象
	m_D3DFont.CreateFont(LogFont,0L);

	LOGFONT LogRollFont;
	ZeroMemory(&LogRollFont,sizeof(LogRollFont));
	LogRollFont.lfHeight=20;
	_sntprintf(LogRollFont.lfFaceName,CountArray(LogRollFont.lfFaceName),TEXT("黑体"));
	LogRollFont.lfWeight=200;
	LogRollFont.lfCharSet=134;
	m_D3DRollFont.CreateFont(LogRollFont,0L);

	//虚拟设备
	m_VirtualEngine.SetD3DDevice(&m_D3DDevice);

	//设置头像
	CGameFrameAvatar * pGameFrameAvatar=CGameFrameAvatar::GetInstance();
	if (pGameFrameAvatar!=NULL) pGameFrameAvatar->Initialization(pD3DDevice);

	//加载资源
	HINSTANCE hInstance=GetModuleHandle(GAME_FRAME_DLL_NAME);
	m_D3DTextureReady.LoadImage(pD3DDevice,hInstance,TEXT("USER_READY"),TEXT("PNG"));
	m_D3DTextureMember.LoadImage(pD3DDevice,hInstance,TEXT("MEMBER_FLAG"),TEXT("PNG"));
	m_D3DTextureClockItem.LoadImage(pD3DDevice,hInstance,TEXT("USER_CLOCK_ITEM"),TEXT("PNG"));
	m_D3DTextureClockBack.LoadImage(pD3DDevice,hInstance,TEXT("USER_CLOCK_BACK"),TEXT("PNG"));

	//配置窗口
	InitGameView(pD3DDevice,nWidth,nHeight);

	return;
}

//丢失设备
VOID CGameFrameViewD3D::OnLostDevice(CD3DDevice * pD3DDevice, INT nWidth, INT nHeight)
{
	return;
}

//重置设备
VOID CGameFrameViewD3D::OnResetDevice(CD3DDevice * pD3DDevice, INT nWidth, INT nHeight)
{
	return;
}

//渲染窗口
VOID CGameFrameViewD3D::OnRenderWindow(CD3DDevice * pD3DDevice, INT nWidth, INT nHeight)
{
	//绘画窗口
	DrawGameView(pD3DDevice,nWidth,nHeight);

	//绘画比赛
	DrawMatchInfo(pD3DDevice);

	//虚拟框架
	m_VirtualEngine.DrawWindows(pD3DDevice);

	//等待提示
	if(m_bShowWaitDistribute)
	{
		CD3DSprite ImageWaitDistribute;
		ImageWaitDistribute.LoadImage(pD3DDevice,GetModuleHandle(GAME_FRAME_DLL_NAME),TEXT("WAIT_DISTRIBUTE"),TEXT("PNG"));
		ImageWaitDistribute.DrawImage(pD3DDevice,(nWidth-ImageWaitDistribute.GetWidth())/2,(nHeight-ImageWaitDistribute.GetHeight())/2);
	}

	//设置变量
	m_dwDrawCurrentCount++;

	//累计判断
	if ((GetTickCount()-m_dwDrawBenchmark)>=1000L)
	{
		//设置变量
		m_dwDrawLastCount=m_dwDrawCurrentCount;

		//统计还原
		m_dwDrawCurrentCount=0L;
		m_dwDrawBenchmark=GetTickCount();
	}

	return;
}

//消息解释
BOOL CGameFrameViewD3D::PreTranslateMessage(MSG * pMsg)
{
	//虚拟框架
	if (m_VirtualEngine.PreTranslateMessage(pMsg->message,pMsg->wParam,pMsg->lParam)==true)
	{
		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}

//消息解释
LRESULT CGameFrameViewD3D::DefWindowProc(UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	//虚拟框架
	if (m_VirtualEngine.DefWindowProc(nMessage,wParam,lParam)==true)
	{
		return 0L;
	}

	return __super::DefWindowProc(nMessage,wParam,lParam);
}

//消息转换
VOID CGameFrameViewD3D::RenderGameView()
{
	//渲染设备
	if (m_bInitD3D==true && m_hWnd!=NULL)
	{
		static DWORD nLastRenderTime=timeGetTime();

		//渲染等待
		if(nLastRenderTime<=15) return;

		//发送消息
		nLastRenderTime = timeGetTime();

		//驱动因子
		CLapseCount::PerformLapseCount();

		//动画驱动
		CartoonMovie();

		//绘画界面
		m_D3DDevice.RenderD3DDevice();
	}

	return ;
}

//渲染线程
VOID CGameFrameViewD3D::StartRenderThread()
{
	//效验状态
	ASSERT(m_hRenderThread==NULL);
	if (m_hRenderThread!=NULL) return;

	//创建线程
	m_hRenderThread=(HANDLE)::CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)D3DRenderThread,(LPVOID)(m_hWnd),0L,0L);

	return;
}

//绘画准备
VOID CGameFrameViewD3D::DrawUserReady(CD3DDevice * pD3DDevice, INT nXPos, INT nYPos)
{
	//获取大小
	CSize SizeUserReady;
	SizeUserReady.SetSize(m_D3DTextureReady.GetWidth(),m_D3DTextureReady.GetHeight());

	//绘画准备
	m_D3DTextureReady.DrawImage(pD3DDevice,nXPos-SizeUserReady.cx/2,nYPos-SizeUserReady.cy/2);

	return;
}

//绘画标志
VOID CGameFrameViewD3D::DrawOrderFlag(CD3DDevice * pD3DDevice, INT nXPos, INT nYPos, BYTE cbImageIndex)
{
	//获取大小
	CSize SizeMember;
	SizeMember.SetSize(m_D3DTextureMember.GetWidth()/3,m_D3DTextureMember.GetHeight());

	//绘画标志
	m_D3DTextureMember.DrawImage(pD3DDevice,nXPos,nYPos,SizeMember.cx,SizeMember.cy,SizeMember.cx*cbImageIndex,0);

	return;
}

//绘画时间
VOID CGameFrameViewD3D::DrawUserClock(CD3DDevice * pD3DDevice, INT nXPos, INT nYPos, WORD wUserClock)
{
	//绘画时间
	if ((wUserClock>0)&&(wUserClock<100))
	{
		//获取大小
		CSize SizeClockItem;
		CSize SizeClockBack;
		SizeClockBack.SetSize(m_D3DTextureClockBack.GetWidth(),m_D3DTextureClockBack.GetHeight());
		SizeClockItem.SetSize(m_D3DTextureClockItem.GetWidth()/10,m_D3DTextureClockItem.GetHeight());

		//绘画背景
		INT nXDrawPos=nXPos-SizeClockBack.cx/2;
		INT nYDrawPos=nYPos-SizeClockBack.cy/2;
		m_D3DTextureClockBack.DrawImage(pD3DDevice,nXDrawPos,nYDrawPos);

		//绘画时间
		WORD nClockItem1=wUserClock/10;
		WORD nClockItem2=wUserClock%10;
		m_D3DTextureClockItem.DrawImage(pD3DDevice,nXDrawPos+16,nYDrawPos+29,SizeClockItem.cx,SizeClockItem.cy,nClockItem1*SizeClockItem.cx,0);
		m_D3DTextureClockItem.DrawImage(pD3DDevice,nXDrawPos+30,nYDrawPos+29,SizeClockItem.cx,SizeClockItem.cy,nClockItem2*SizeClockItem.cx,0);
	}

	return;
}

//绘画头像
VOID CGameFrameViewD3D::DrawUserAvatar(CD3DDevice * pD3DDevice, INT nXPos, INT nYPos, IClientUserItem * pIClientUserItem)
{
	//设置头像
	CGameFrameAvatar * pGameFrameAvatar=CGameFrameAvatar::GetInstance();
	if (pGameFrameAvatar!=NULL) pGameFrameAvatar->DrawUserAvatar(pD3DDevice,nXPos,nYPos,pIClientUserItem);

	return;
}

//绘画头像
VOID CGameFrameViewD3D::DrawUserAvatar(CD3DDevice * pD3DDevice, INT nXPos, INT nYPos, INT nWidth, INT nHeight, IClientUserItem * pIClientUserItem)
{
	//设置头像
	CGameFrameAvatar * pGameFrameAvatar=CGameFrameAvatar::GetInstance();
	if (pGameFrameAvatar!=NULL) pGameFrameAvatar->DrawUserAvatar(pD3DDevice,nXPos,nYPos,nWidth,nHeight,pIClientUserItem);

	return;
}

//绘画背景
VOID CGameFrameViewD3D::DrawViewImage(CD3DDevice * pD3DDevice, CD3DTexture & D3DTexture, BYTE cbDrawMode)
{
	//获取位置
	CRect rcClient;
	GetClientRect(&rcClient);

	//绘画位图
	switch (cbDrawMode)
	{
	case DRAW_MODE_SPREAD:		//平铺模式
		{
			//获取大小
			CSize SizeTexture;
			SizeTexture.SetSize(D3DTexture.GetWidth(),D3DTexture.GetHeight());

			//绘画位图
			for (INT nXPos=0;nXPos<rcClient.right;nXPos+=SizeTexture.cx)
			{
				for (INT nYPos=0;nYPos<rcClient.bottom;nYPos+=SizeTexture.cy)
				{
					D3DTexture.DrawImage(pD3DDevice,nXPos,nYPos);
				}
			}

			return;
		}
	case DRAW_MODE_CENTENT:		//居中模式
		{
			//获取大小
			CSize SizeTexture;
			SizeTexture.SetSize(D3DTexture.GetWidth(),D3DTexture.GetHeight());

			//位置计算
			INT nXPos=(rcClient.Width()-SizeTexture.cx)/2;
			INT nYPos=(rcClient.Height()-SizeTexture.cy)/2;

			//绘画位图
			D3DTexture.DrawImage(pD3DDevice,nXPos,nYPos);

			return;
		}
	case DRAW_MODE_ELONGGATE:	//拉伸模式
		{
			//获取大小
			CSize SizeTexture;
			SizeTexture.SetSize(D3DTexture.GetWidth(),D3DTexture.GetHeight());

			//绘画位图
			D3DTexture.DrawImage(pD3DDevice,0,0,rcClient.Width(),rcClient.Height(),0,0,SizeTexture.cx,SizeTexture.cy);

			return;
		}
	}

	return;
}

//绘画数字
VOID CGameFrameViewD3D::DrawNumberString(CD3DDevice * pD3DDevice, CD3DTexture & D3DTexture, LONG lNumber, INT nXPos, INT nYPos)
{
	//效验状态
	ASSERT(D3DTexture.IsNull()==false);
	if (D3DTexture.IsNull()==true) return;

	//获取大小
	CSize SizeNumber;
	SizeNumber.SetSize(D3DTexture.GetWidth()/10,D3DTexture.GetHeight());

	//变量定义
	LONG lNumberCount=0;
	LONG lNumberTemp=lNumber;

	//计算数目
	do
	{
		lNumberCount++;
		lNumberTemp/=10L;
	} while (lNumberTemp>0);

	//位置定义
	INT nYDrawPos=nYPos-SizeNumber.cy/2;
	INT nXDrawPos=nXPos+lNumberCount*SizeNumber.cx/2-SizeNumber.cx;

	//绘画桌号
	for (LONG i=0;i<lNumberCount;i++)
	{
		//绘画号码
		LONG lCellNumber=lNumber%10L;
		D3DTexture.DrawImage(pD3DDevice,nXDrawPos,nYDrawPos,SizeNumber.cx,SizeNumber.cy,lCellNumber*SizeNumber.cx,0);

		//设置变量
		lNumber/=10;
		nXDrawPos-=SizeNumber.cx;
	};

	return;
}

//绘画背景
VOID CGameFrameViewD3D::DrawViewImage(CD3DDevice * pD3DDevice, CD3DSprite & D3DSprite, BYTE cbDrawMode)
{
	//获取位置
	CRect rcClient;
	GetClientRect(&rcClient);

	//绘画位图
	switch (cbDrawMode)
	{
	case DRAW_MODE_SPREAD:		//平铺模式
		{
			//获取大小
			CSize SizeTexture;
			SizeTexture.SetSize(D3DSprite.GetWidth(),D3DSprite.GetHeight());

			//绘画位图
			for (INT nXPos=0;nXPos<rcClient.right;nXPos+=SizeTexture.cx)
			{
				for (INT nYPos=0;nYPos<rcClient.bottom;nYPos+=SizeTexture.cy)
				{
					D3DSprite.DrawImage(pD3DDevice,nXPos,nYPos);
				}
			}

			return;
		}
	case DRAW_MODE_CENTENT:		//居中模式
		{
			//获取大小
			CSize SizeTexture;
			SizeTexture.SetSize(D3DSprite.GetWidth(),D3DSprite.GetHeight());

			//位置计算
			INT nXPos=(rcClient.Width()-SizeTexture.cx)/2;
			INT nYPos=(rcClient.Height()-SizeTexture.cy)/2;

			//绘画位图
			D3DSprite.DrawImage(pD3DDevice,nXPos,nYPos);

			return;
		}
	case DRAW_MODE_ELONGGATE:	//拉伸模式
		{
			//获取大小
			CSize SizeTexture;
			SizeTexture.SetSize(D3DSprite.GetWidth(),D3DSprite.GetHeight());

			//绘画位图
			D3DSprite.DrawImage(pD3DDevice,0,0,rcClient.Width(),rcClient.Height(),0,0,SizeTexture.cx,SizeTexture.cy);

			return;
		}
	}

	return;
}

//绘画数字
VOID CGameFrameViewD3D::DrawNumberString(CD3DDevice * pD3DDevice, CD3DSprite & D3DSprite, LONG lNumber, INT nXPos, INT nYPos)
{
	//效验状态
	ASSERT(D3DSprite.IsNull()==false);
	if (D3DSprite.IsNull()==true) return;

	//获取大小
	CSize SizeNumber;
	SizeNumber.SetSize(D3DSprite.GetWidth()/10,D3DSprite.GetHeight());

	//变量定义
	LONG lNumberCount=0;
	LONG lNumberTemp=lNumber;

	//计算数目
	do
	{
		lNumberCount++;
		lNumberTemp/=10L;
	} while (lNumberTemp>0);

	//位置定义
	INT nYDrawPos=nYPos-SizeNumber.cy/2;
	INT nXDrawPos=nXPos+lNumberCount*SizeNumber.cx/2-SizeNumber.cx;

	//绘画桌号
	for (LONG i=0;i<lNumberCount;i++)
	{
		//绘画号码
		LONG lCellNumber=lNumber%10L;
		D3DSprite.DrawImage(pD3DDevice,nXDrawPos,nYDrawPos,SizeNumber.cx,SizeNumber.cy,lCellNumber*SizeNumber.cx,0);

		//设置变量
		lNumber/=10;
		nXDrawPos-=SizeNumber.cx;
	};

	return;
}

//绘画字符
VOID CGameFrameViewD3D::DrawTextString(CD3DDevice * pD3DDevice, LPCTSTR pszString, CRect rcDraw, UINT nFormat, D3DCOLOR crText, D3DCOLOR crFrame)
{
	//变量定义
	INT nXExcursion[8]={1,1,1,0,-1,-1,-1,0};
	INT nYExcursion[8]={-1,0,1,1,1,0,-1,-1};

	//绘画边框
	for (INT i=0;i<CountArray(nXExcursion);i++)
	{
		//计算位置
		CRect rcFrame;
		rcFrame.top=rcDraw.top+nYExcursion[i];
		rcFrame.left=rcDraw.left+nXExcursion[i];
		rcFrame.right=rcDraw.right+nXExcursion[i];
		rcFrame.bottom=rcDraw.bottom+nYExcursion[i];

		//绘画字符
		m_D3DFont.DrawText(pD3DDevice,pszString,&rcFrame,nFormat,crFrame);
	}

	//绘画字符
	m_D3DFont.DrawText(pD3DDevice,pszString,&rcDraw,nFormat,crText);

	return;
}

//绘画字符
VOID CGameFrameViewD3D::DrawTextString(CD3DDevice * pD3DDevice, LPCTSTR pszString, INT nXPos, INT nYPos, UINT nFormat, D3DCOLOR crText, D3DCOLOR crFrame)
{
	//变量定义
	INT nXExcursion[8]={1,1,1,0,-1,-1,-1,0};
	INT nYExcursion[8]={-1,0,1,1,1,0,-1,-1};

	//绘画边框
	for (INT i=0;i<CountArray(nXExcursion);i++)
	{
		m_D3DFont.DrawText(pD3DDevice,pszString,nXPos+nXExcursion[i],nYPos+nYExcursion[i],nFormat,crFrame);
	}

	//绘画字符
	m_D3DFont.DrawText(pD3DDevice,pszString,nXPos,nYPos,nFormat,crText);

	return;
}

//输出字体
VOID CGameFrameViewD3D::DrawTextString(CD3DDevice * pD3DDevice, LPCTSTR pszString, CRect rcDraw,UINT uFormat, D3DCOLOR D3DColor)
{
	//输出字体
	m_D3DFont.DrawText(pD3DDevice,pszString,rcDraw,uFormat,D3DColor);

	return;
}

//输出字体
VOID CGameFrameViewD3D::DrawTextString(CD3DDevice * pD3DDevice, LPCTSTR pszString, INT nXPos, INT nYPos, UINT uFormat, D3DCOLOR D3DColor)
{
	//输出字体
	m_D3DFont.DrawText(pD3DDevice,pszString,nXPos,nYPos,uFormat,D3DColor);

	return;
}

//绘画函数
VOID CGameFrameViewD3D::OnPaint()
{
	CPaintDC dc(this);

	//渲染设备
	if (m_bInitD3D==true) 
	{
		//驱动因子
		CLapseCount::PerformLapseCount();

		//动画驱动
		CartoonMovie();

		//绘画界面
		m_D3DDevice.RenderD3DDevice();
	}

	return;
}

//位置变化
VOID CGameFrameViewD3D::OnSize(UINT nType, INT cx, INT cy)
{
	//重置设备
	if (m_bInitD3D==false)
	{
		if ((cx>=0L)&&(cy>0L))
		{
			//配置环境
			if (m_D3DDirect.CreateD3DDirect()==false)
			{
				ASSERT(FALSE);
				return;
			}

			//创建设备
			if (m_D3DDevice.CreateD3DDevice(m_hWnd,this)==false)
			{
				ASSERT(FALSE);
				return;
			}
		}
	}
	else
	{
		//重置设备
		m_D3DDevice.ResetD3DDevice();
	}

	__super::OnSize(nType, cx, cy);
}

//渲染消息
LRESULT CGameFrameViewD3D::OnMessageD3DRender(WPARAM wParam, LPARAM lParam)
{
	//渲染设备
	if (m_bInitD3D==true)
	{
		//驱动因子
		CLapseCount::PerformLapseCount();

		//动画驱动
		CartoonMovie();

		//绘画界面
		m_D3DDevice.RenderD3DDevice();
	}

	return 0L;
}

//渲染线程
VOID CGameFrameViewD3D::D3DRenderThread(LPVOID pThreadData)
{
	//效验参数
	ASSERT(pThreadData!=NULL);
	if (pThreadData==NULL) return;

	int nPaintingTime = 0;
	//渲染循环
	while (TRUE)
	{
		//渲染等待
		if( nPaintingTime >= 15 )
		{
			Sleep(1);
		}
		else
		{
			Sleep(15 - nPaintingTime);
		}
		
		//发送消息
		int nTime = timeGetTime();
		::SendMessage((HWND)pThreadData,WM_D3D_RENDER,0L,0L);
		nPaintingTime = timeGetTime() - nTime;

		TRACE(TEXT(" F [%d] \n"), nPaintingTime);
	}

	return;
}

//桌面消息
void CGameFrameViewD3D::AddGameTableMessage(LPCTSTR pszMessage,WORD wLen,WORD wType)
{
	if(wType&SMT_TABLE_ROLL)
	{
		m_strRollText=CString(pszMessage,wLen);
		if(m_strRollText.Find(TEXT("系统配桌"))>=0)m_bShowWaitDistribute=TRUE;
		if(m_vecText.empty())
		{
			m_wRollTextCount=160;
			SetTimer(IDI_ROLL_TEXT,20,NULL);
		}
		m_vecText.push_back(m_strRollText);
	}
	// 	else if(wType&SMT_TABLE_FIX)
	// 	{
	// 		m_strFixText="";
	// 		m_strFixText=CString(pszMessage,wLen);
	// 		UpdateGameView(&CRect(m_wTextLeft,400,m_wTextLeft+500,600));
	// 	}

}


//比赛信息
VOID CGameFrameViewD3D::DrawMatchInfo(CD3DDevice * pD3DDevice)
{
	//窗口尺寸
	CRect rcClient;
	GetClientRect(&rcClient);

	//等待提示
	if(m_bShowWaitDistribute)
	{
		CD3DSprite ImageWaitDistribute;
		ImageWaitDistribute.LoadImage(pD3DDevice, GetModuleHandle(GAME_FRAME_DLL_NAME),TEXT("WAIT_DISTRIBUTE"),TEXT("PNG"));
		ImageWaitDistribute.DrawImage(pD3DDevice,(rcClient.Width()-ImageWaitDistribute.GetWidth())/2,(rcClient.Height()-ImageWaitDistribute.GetHeight())/2);
	}

	//比赛信息
	if(m_pMatchInfo!=NULL)
	{
		CD3DSprite ImageMatchInfo;
		ImageMatchInfo.LoadImage(pD3DDevice,GetModuleHandle(GAME_FRAME_DLL_NAME),TEXT("MATCH_INFO"),TEXT("PNG"));
		ImageMatchInfo.DrawImage(pD3DDevice,0,0,(BYTE)210);
		
		CString strNum;
		strNum.Format(TEXT("%d"),m_pMatchInfo->wGameCount);
		DrawTextString(pD3DDevice,strNum,12,32,DT_LEFT,D3DCOLOR_XRGB(255,255,255));
		
		DrawTextString(pD3DDevice,m_pMatchInfo->szTitle[0],38,10,DT_LEFT,D3DCOLOR_XRGB(196,221,239));
		//DrawTextString(pD3DDevice,m_pMatchInfo->szTitle[1],38,30,DT_LEFT,D3DCOLOR_XRGB(196,221,239));
		DrawTextString(pD3DDevice,m_pMatchInfo->szTitle[2],38,30,DT_LEFT,D3DCOLOR_XRGB(196,221,239));
		DrawTextString(pD3DDevice,m_pMatchInfo->szTitle[3],38,50,DT_LEFT,D3DCOLOR_XRGB(255,221,35));
	}

	//等待提示
	if(m_pMatchWaitTip!=NULL)
	{
		//定义变量
		CD3DSprite ImageWaitTip;
		CD3DSprite ImageNumberOrange;
		CD3DSprite ImageNumberGreen;
		CD3DSprite ImageLine;
		HMODULE hModule=GetModuleHandle(GAME_FRAME_DLL_NAME);

		//加载图片
		ImageWaitTip.LoadImage(pD3DDevice,hModule,TEXT("MATCH_WAIT_TIP"),TEXT("PNG"));				
		ImageNumberOrange.LoadImage(pD3DDevice,hModule,TEXT("NUMBER_ORANGE"),TEXT("PNG"));
		ImageNumberGreen.LoadImage(pD3DDevice,hModule, TEXT("NUMBER_GREEN"),TEXT("PNG"));
		ImageLine.LoadImage(pD3DDevice,hModule, TEXT("MATCH_LINE"),TEXT("PNG"));

		//计算位置
		INT nXPos=(rcClient.Width()-ImageWaitTip.GetWidth())/2;
		INT nYPos=(rcClient.Height()-ImageWaitTip.GetHeight())-20;

		//获取字体
		LOGFONT LogFont;
		ZeroMemory(&LogFont,sizeof(LogFont));
		LogFont.lfHeight=22;
		LogFont.lfWeight=200;

		//创建字体
		CD3DFont DrawFont;
		DrawFont.CreateFont(LogFont,0L);
		

		//绘画信息
		ImageWaitTip.DrawImage(pD3DDevice,nXPos,nYPos-20,(BYTE)220);
		DrawFont.DrawText(pD3DDevice,m_pMatchWaitTip->szMatchName,&CRect(nXPos+20,nYPos+10, nXPos+ImageWaitTip.GetWidth()-40,nYPos+40),
			DT_VCENTER|DT_SINGLELINE|DT_CENTER,D3DCOLOR_XRGB(255,225,0));
		DrawFont.DeleteFont();

		//输出文字
		CString strDrawText;
		D3DCOLOR clrText=D3DCOLOR_XRGB(200,246,244);
		strDrawText.Format(TEXT("您目前积分：                    排名："));
		m_D3DFont.DrawText(pD3DDevice,strDrawText, &CRect(nXPos+18,nYPos+64, nXPos+ImageWaitTip.GetWidth()-36, nYPos+90),
			DT_TOP|DT_SINGLELINE|DT_END_ELLIPSIS,clrText);

		DrawNumber(pD3DDevice,nXPos+150, nYPos+68, &ImageNumberOrange, TEXT("0123456789-"), (LONG)m_pMatchWaitTip->lScore, TEXT("%+d"), DT_CENTER|DT_VCENTER);
		DrawNumber(pD3DDevice,nXPos+262, nYPos+68, &ImageNumberOrange, TEXT("0123456789-"), m_pMatchWaitTip->wRank, TEXT("%+d"), DT_CENTER|DT_VCENTER);

		//DrawNumberString(pD3DDevice, ImageNumberOrange, m_pMatchWaitTip->wRank, nXPos+260,nYPos+68); 

		ImageLine.DrawImage(pD3DDevice, nXPos+272, nYPos+57);

		DrawNumber(pD3DDevice,nXPos+295, nYPos+68, &ImageNumberOrange, TEXT("0123456789-"), m_pMatchWaitTip->wUserCount, TEXT("%+d"), DT_LEFT|DT_VCENTER);
		//DrawNumberString(pD3DDevice, ImageNumberOrange, m_pMatchWaitTip->wUserCount, nXPos+303,nYPos+68/*,TA_RIGHT*/);
		strDrawText.Format(TEXT("请耐心等待，还有      桌未完成比赛"));
		m_D3DFont.DrawText(pD3DDevice,strDrawText, &CRect(nXPos+18,nYPos+104, nXPos+ImageWaitTip.GetWidth()-36, nYPos+130),
			DT_TOP|DT_SINGLELINE|DT_END_ELLIPSIS,clrText);
		DrawNumberString(pD3DDevice, ImageNumberGreen, m_pMatchWaitTip->wPlayingTable, nXPos+130,nYPos+108/*,TA_RIGHT*/);
		strDrawText.Format(TEXT("您已经率先完成第             局，处于本桌第      名。"));
		m_D3DFont.DrawText(pD3DDevice,strDrawText, &CRect(nXPos+18,nYPos+144, nXPos+ImageWaitTip.GetWidth()-36, nYPos+200),
			DT_TOP|DT_WORDBREAK,clrText);


		DrawNumber(pD3DDevice,nXPos+135, nYPos+148, &ImageNumberOrange, TEXT("0123456789-"), m_pMatchWaitTip->wCurGameCount, TEXT("%+d"), DT_CENTER|DT_VCENTER);
		//DrawNumberString(pD3DDevice, ImageNumberOrange, m_pMatchWaitTip->wCurGameCount, nXPos+125,nYPos+148); 
		ImageLine.DrawImage(pD3DDevice, nXPos+138, nYPos+140);

		DrawNumber(pD3DDevice,nXPos+154, nYPos+148, &ImageNumberOrange, TEXT("0123456789-"), m_pMatchWaitTip->wGameCount, TEXT("%+d"), DT_LEFT|DT_VCENTER);
		//DrawNumberString(pD3DDevice, ImageNumberOrange, m_pMatchWaitTip->wGameCount, nXPos+170,nYPos+148/*,TA_RIGHT*/);
		DrawNumberString(pD3DDevice,ImageNumberGreen,m_pMatchWaitTip->wCurTableRank,nXPos+290,nYPos+148);
		m_D3DFont.DrawText(pD3DDevice,TEXT("请等待其他桌游戏结束后，确定最后排名。"), &CRect(nXPos+18,nYPos+188, nXPos+ImageWaitTip.GetWidth()-36, nYPos+220),
			DT_TOP|DT_WORDBREAK,clrText);
		
	}

	if (m_strRollText.GetLength()>0)
	{
		//变量定义
		TCHAR szMsg[256]=TEXT("");
		_sntprintf(szMsg,CountArray(szMsg),TEXT("%s"),m_strRollText);
		TCHAR szSub1[64]=TEXT("");
		TCHAR szSub2[128]=TEXT("");
		TCHAR szSub3[256]=TEXT("");
		TCHAR *pSub[]={szSub1,szSub2,szSub3};
		int nPos=0;
		int nTextLine=0;
		INT nLen=m_strRollText.GetLength();
		int  nWide=sizeof(TCHAR)==sizeof(CHAR)?2:1;

		for (INT i=0;i<nLen;)
		{
			i+=((BYTE)(szMsg[i])>=128)?2:1;
			if ((pSub[nTextLine][0]=='\0')&&(i>=(20*(nTextLine+1)*nWide)))
			{
				_tcsncpy(pSub[nTextLine++],&szMsg[nPos],i-nPos);
				nPos=i;
				if(nTextLine==CountArray(pSub))
				{
					nPos=nLen;
					break;
				}
			}
		}
		if(nPos<nLen)
		{
			_tcsncpy(pSub[nTextLine++],&szMsg[nPos],nLen-nPos);
		}
		
 		if (nTextLine==3)
 		{
 			m_D3DRollFont.DrawText(pD3DDevice,szSub1, &m_rcText,DT_CENTER,D3DCOLOR_XRGB(255,255,255));
 			CRect rcLine=m_rcText;
 			rcLine.top+=25;
 			if (rcLine.top<rcLine.bottom)
 				m_D3DRollFont.DrawText(pD3DDevice,szSub2, &rcLine,DT_CENTER,D3DCOLOR_XRGB(255,255,255));
 			rcLine.top+=25;
 			if(rcLine.top<rcLine.bottom)
 				m_D3DRollFont.DrawText(pD3DDevice,szSub3, &rcLine,DT_CENTER,D3DCOLOR_XRGB(255,255,255));
 		}
 		else if(nTextLine==2)
 		{
 			m_D3DRollFont.DrawText(pD3DDevice,szSub1, &m_rcText,DT_CENTER,D3DCOLOR_XRGB(255,255,255));
 			CRect rcLine=m_rcText;
 			rcLine.top+=25;
 			if(rcLine.top<rcLine.bottom)
 				m_D3DRollFont.DrawText(pD3DDevice,szSub2, &rcLine,DT_CENTER,D3DCOLOR_XRGB(255,255,255));
 		}
 		else
 		{
 			m_D3DRollFont.DrawText(pD3DDevice,m_strRollText, &m_rcText,DT_CENTER,D3DCOLOR_XRGB(255,255,255));
 		}
	}
}

void CGameFrameViewD3D::OnTimer(UINT nIDEvent)
{
	if(IDI_ROLL_TEXT==nIDEvent)
	{	
		m_wRollTextCount--;

		LONG lLeft=m_rcText.left;
		if(m_wRollTextCount<50)
		{
			m_rcText=CRect(m_wTextLeft,319,m_wTextLeft+500,319+m_wRollTextCount*2);
		}
		else if(m_wRollTextCount>=110)
		{
			m_rcText=CRect(m_wTextLeft,430-(160-m_wRollTextCount),m_wTextLeft+500,430);
		}
		else
		{
			m_rcText=CRect(m_wTextLeft,m_wRollTextCount+270,m_wTextLeft+500,m_wRollTextCount+370);
		}	

		if(m_wRollTextCount==0)
		{
			if(m_vecText.size()<=1)
			{
				InvalidateRect(NULL);
				m_vecText.clear();
				KillTimer(IDI_ROLL_TEXT);
				m_wRollTextCount=160;
				m_strRollText="";
				m_rcText.SetRect(0,0,1,1);
			}
			else
			{
				InvalidateRect(NULL);
				vector <CString>::iterator Iter;
				Iter=m_vecText.begin();
				m_vecText.erase(Iter);
				m_strRollText=m_vecText.at(0);
				m_wRollTextCount=160;
				m_rcText=CRect(m_wTextLeft,m_wRollTextCount+270,m_wTextLeft+500,m_wRollTextCount+370);
			}

		}
	}
	__super::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////////////////
