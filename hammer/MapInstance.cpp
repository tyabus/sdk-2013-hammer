#include "stdafx.h"
#include "MapInstance.h"
#include "mapentity.h"
#include "mapdoc.h"
#include "mapworld.h"
#include "render2d.h"
#include "Render3DMS.h"

#include "smartptr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

IMPLEMENT_MAPCLASS( CMapInstance );

CMapClass* CMapInstance::Create( CHelperInfo* pHelperInfo, CMapEntity* pParent )
{
	if ( const char* file = pParent->GetKeyValue( "file" ) )
		return new CMapInstance( file );
	else
		return new CMapInstance;
}

CMapInstance::CMapInstance() : m_pTemplate( NULL )
{
}

CMapInstance::CMapInstance( const char* pszMapPath ) : m_pTemplate( static_cast<CMapDoc*>( CMapDoc::CreateObject() ) ), m_strCurrentVMF( pszMapPath )
{
	m_pTemplate->LoadVMF( pszMapPath, true );
}

CMapInstance::~CMapInstance()
{
	if ( m_pTemplate )
		m_pTemplate->OnCloseDocument();
}

CMapClass* CMapInstance::Copy( bool bUpdateDependencies )
{
	CMapInstance* inst = new CMapInstance;
	if (inst != NULL)
		inst->CopyFrom( this, bUpdateDependencies );
	return inst;
}

CMapClass* CMapInstance::CopyFrom( CMapClass* pFrom, bool bUpdateDependencies )
{
	Assert( pFrom->IsMapClass( MAPCLASS_TYPE( CMapInstance ) ) );
	CMapInstance* pObject = static_cast<CMapInstance*>( pFrom );

	CMapClass::CopyFrom( pObject, bUpdateDependencies );

	m_strCurrentVMF = pObject->m_strCurrentVMF;

	return this;
}

void CMapInstance::SetOrigin( Vector& pfOrigin )
{
	CMapHelper::SetOrigin( pfOrigin );
}

void CMapInstance::SetCullBoxFromFaceList( CMapFaceList* pFaces )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->SetCullBoxFromFaceList( pFaces );
	else
		CMapHelper::SetCullBoxFromFaceList( pFaces );
}

void CMapInstance::CalcBounds( BOOL bFullUpdate )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->CalcBounds( bFullUpdate );
	else
		CMapHelper::CalcBounds( bFullUpdate );
}

void CMapInstance::GetCullBox( Vector& mins, Vector& maxs )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->GetCullBox( mins, maxs );
	else
		CMapHelper::GetCullBox( mins, maxs );
}

bool CMapInstance::GetCullBoxChild( Vector& mins, Vector& maxs )
{
	GetCullBox( mins, maxs );
	return true;
}

void CMapInstance::GetRender2DBox( Vector& mins, Vector& maxs )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->GetRender2DBox( mins, maxs );
	else
		CMapHelper::GetRender2DBox( mins, maxs );
}

bool CMapInstance::GetRender2DBoxChild( Vector& mins, Vector& maxs )
{
	GetRender2DBox( mins, maxs );
	return true;
}

void CMapInstance::GetBoundsCenter( Vector& vecCenter )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->GetBoundsCenter( vecCenter );
	else
		CMapHelper::GetBoundsCenter( vecCenter );
}

bool CMapInstance::GetBoundsCenterChild( Vector & vecCenter )
{
	GetBoundsCenter( vecCenter );
	return true;
}

void CMapInstance::GetBoundsSize( Vector& vecSize )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->GetBoundsSize( vecSize );
	else
		CMapHelper::GetBoundsSize( vecSize );
}

bool CMapInstance::GetBoundsSizeChild( Vector & vecSize )
{
	GetBoundsSize( vecSize );
	return true;
}

void CMapInstance::DoTransform( const VMatrix& matrix )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->DoTransform( matrix );
	CMapHelper::DoTransform( matrix );
}

bool CMapInstance::PostloadVisGroups( bool bIsLoading )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
	{
		CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );

		m_pTemplate->GetMapWorld()->PostloadVisGroups();
	}

	return CMapHelper::PostloadVisGroups( bIsLoading );
}

bool CMapInstance::HitTest2D( CMapView2D* pView, const Vector2D& point, HitInfo_t& HitData )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		return m_pTemplate->GetMapWorld()->HitTest2D( pView, point, HitData );
	else
		return CMapHelper::HitTest2D( pView, point, HitData );
}

bool CMapInstance::IsCulledByCordon( const Vector& vecMins, const Vector& vecMaxs )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		return m_pTemplate->GetMapWorld()->IsCulledByCordon( vecMins, vecMaxs );
	else
		return CMapHelper::IsCulledByCordon( vecMins, vecMaxs );
}

bool CMapInstance::IsInsideBox( Vector const& Mins, Vector const& Maxs ) const
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		return m_pTemplate->GetMapWorld()->IsInsideBox( Mins, Maxs );
	else
		return CMapHelper::IsInsideBox( Mins, Maxs );
}

bool CMapInstance::IsIntersectingBox( const Vector& vecMins, const Vector& vecMaxs ) const
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		return m_pTemplate->GetMapWorld()->IsIntersectingBox( vecMins, vecMaxs );
	else
		return CMapHelper::IsIntersectingBox( vecMins, vecMaxs );
}

void CMapInstance::OnParentKeyChanged( const char* key, const char* value )
{
	if ( !stricmp( key, "file" ) && ( !m_strCurrentVMF.IsEqual_CaseInsensitive( value ) || !m_pTemplate ) )
	{
		if ( !m_pTemplate )
			m_pTemplate = static_cast<CMapDoc*>( CMapDoc::CreateObject() );
		else
			m_pTemplate->DeleteCurrentMap();
		m_strCurrentVMF.Set( value );
		if ( !m_strCurrentVMF.IsEmpty() )
		{
			m_pTemplate->LoadVMF( value, true );
			Vector origin;
			GetOrigin( origin );
			m_pTemplate->GetMapWorld()->TransMove( origin );
			m_pTemplate->GetMapWorld()->SetRenderColor( 64, 192, 255 );
		}
		PostUpdate(Notify_Changed);
	}
}

void CMapInstance::Render2D( CRender2D* pRender )
{
	if ( !m_pTemplate || !m_pTemplate->GetMapWorld() )
		return;

	CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );

	m_pTemplate->GetMapWorld()->SetRenderColor( 64, 192, 255 );
	CMapObjectList& children = m_pTemplate->GetMapWorld()->m_Children;
	for( CMapClass* pChild : children )
	{
		if (pChild->IsVisible() && pChild->IsVisible2D())
		{
			pChild->Render2D(pRender);
		}
	}
}

void CMapInstance::Render3D( CRender3D* pRender )
{
	if ( m_pTemplate )
	{
		CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );
		CAutoPushPop<bool> guard2( pRender->m_DeferRendering, false );

		m_pTemplate->GetMapWorld()->SetRenderColor( 64, 192, 255 );
		pRender->RenderMapClass( m_pTemplate->GetMapWorld() );
	}
}

bool CMapInstance::RenderPreload( CRender3D* pRender, bool bNewContext )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
	{
		CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );

		return m_pTemplate->GetMapWorld()->RenderPreload( pRender, bNewContext );
	}
	else
		return CMapHelper::RenderPreload( pRender, bNewContext );
}

void CMapInstance::AddShadowingTriangles( CUtlVector<Vector>& tri_list )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->AddShadowingTriangles( tri_list );
}
