#include "stdafx.h"
#include "MapInstance.h"
#include "mapentity.h"
#include "mapdoc.h"
#include "mapworld.h"
#include "render2d.h"
#include "Render3DMS.h"
#include "ToolInterface.h"
#include "mapview2d.h"

#include "smartptr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

IMPLEMENT_MAPCLASS( CMapInstance );

CMapClass* CMapInstance::Create( CHelperInfo* pHelperInfo, CMapEntity* pParent )
{
	return new CMapInstance( pParent );
}

CMapInstance::CMapInstance() : m_pTemplate( NULL )
{
}

CMapInstance::CMapInstance( CMapEntity* pParent ) : m_pTemplate( NULL )
{
	m_matTransform.Identity();
	m_strCurrentVMF = pParent->GetKeyValue( "file" );
	if ( !m_strCurrentVMF.IsEmpty() )
	{
		m_pTemplate = static_cast<CMapDoc*>( CMapDoc::CreateObject() );
		CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );
		if ( m_pTemplate->LoadVMF( m_strCurrentVMF, true ) )
		{
			m_pTemplate->GetMapWorld()->SetRenderColor( 134, 130, 0 );
			m_pTemplate->GetMapWorld()->SetModulationColor( Vector( 134 / 255.f, 130 / 255.f, 0 ) );
			m_pTemplate->GetMapWorld()->SetPreferredPickObject( pParent );
		}
	}
}

CMapInstance::~CMapInstance()
{
	if ( m_pTemplate )
		m_pTemplate->OnCloseDocument();
}

CMapClass* CMapInstance::Copy( bool bUpdateDependencies )
{
	CMapInstance* inst = new CMapInstance;
	if ( inst != NULL )
		inst->CopyFrom( this, bUpdateDependencies );
	return inst;
}

CMapClass* CMapInstance::CopyFrom( CMapClass* pFrom, bool bUpdateDependencies )
{
	Assert( pFrom->IsMapClass( MAPCLASS_TYPE( CMapInstance ) ) );
	CMapInstance* pObject = static_cast<CMapInstance*>( pFrom );

	CMapHelper::CopyFrom( pObject, bUpdateDependencies );

	m_strCurrentVMF = pObject->m_strCurrentVMF;
	m_matTransform = pObject->m_matTransform;

	return this;
}

void CMapInstance::OnNotifyDependent( CMapClass* pObject, Notify_Dependent_t eNotifyType )
{
	CMapHelper::OnNotifyDependent( pObject, eNotifyType );
}

void CMapInstance::SetParent( CMapAtom* pParent )
{
	CMapHelper::SetParent( pParent );
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->SetPreferredPickObject( GetParent() );
}

SelectionState_t CMapInstance::SetSelectionState( SelectionState_t eSelectionState )
{
	const SelectionState_t old = CMapHelper::SetSelectionState( eSelectionState );
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
		m_pTemplate->GetMapWorld()->SetSelectionState( eSelectionState == SELECT_NONE ? SELECT_NONE : SELECT_NORMAL );
	return old;
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
		GetBounds( &CMapClass::m_CullBox, mins, maxs );
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
		GetBounds( &CMapClass::m_Render2DBox, mins, maxs );
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
	{
		Vector mins, maxs;
		GetBounds( &CMapClass::m_Render2DBox, mins, maxs );
		VectorLerp( mins, maxs, 0.5f, vecCenter );
	}
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
	{
		Vector mins, maxs;
		GetBounds( &CMapClass::m_Render2DBox, mins, maxs );
		VectorSubtract( maxs, mins, vecSize );
	}
	else
		CMapHelper::GetBoundsSize( vecSize );
}

bool CMapInstance::GetBoundsSizeChild( Vector & vecSize )
{
	GetBoundsSize( vecSize );
	return true;
}

#pragma float_control(precise, on, push)
void CMapInstance::DoTransform( const VMatrix& matrix )
{
	CMapHelper::DoTransform( matrix );

	m_matTransform = matrix * m_matTransform;
	QAngle angle;
	Vector origin;
	DecompressMatrix( origin, angle );
	FixAngles( angle );
	ConstructMatrix( origin, angle );
}
#pragma float_control(pop)

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
	{
		Vector world;
		pView->ClientToWorld( world, point );
		Vector2D transformed;
		pView->WorldToClient( transformed, m_matTransform.InverseTR().VMul4x3( world ) );
		if ( m_pTemplate && m_pTemplate->GetMapWorld() && m_pTemplate->GetMapWorld()->HitTest2D( pView, transformed, HitData ) )
			return true;
	}
	return CMapHelper::HitTest2D( pView, point, HitData );
}

bool CMapInstance::IsCulledByCordon( const Vector& vecMins, const Vector& vecMaxs )
{
	return !IsIntersectingBox( vecMins, vecMaxs );
}

bool CMapInstance::IsInsideBox( Vector const& pfMins, Vector const& pfMaxs ) const
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
	{
		Vector bmins, bmaxs;
		GetBounds( &CMapClass::m_Render2DBox, bmins, bmaxs );

		if ( bmins[0] < pfMins[0] || bmaxs[0] > pfMaxs[0] )
			return CMapHelper::IsInsideBox( pfMins, pfMaxs );

		if ( bmins[1] < pfMins[1] || bmaxs[1] > pfMaxs[1] )
			return CMapHelper::IsInsideBox( pfMins, pfMaxs );

		if ( bmins[2] < pfMins[2] || bmaxs[2] > pfMaxs[2] )
			return CMapHelper::IsInsideBox( pfMins, pfMaxs );

		return true;
	}
	else
		return CMapHelper::IsInsideBox( pfMins, pfMaxs );
}

bool CMapInstance::IsIntersectingBox( const Vector& vecMins, const Vector& vecMaxs ) const
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
	{
		Vector bmins, bmaxs;
		GetBounds( &CMapClass::m_Render2DBox, bmins, bmaxs );

		if ( bmins[0] >= vecMaxs[0] || bmaxs[0] <= vecMins[0] )
			return CMapHelper::IsIntersectingBox( vecMins, vecMaxs );

		if ( bmins[1] >= vecMaxs[1] || bmaxs[1] <= vecMins[1] )
			return CMapHelper::IsIntersectingBox( vecMins, vecMaxs );

		if ( bmins[2] >= vecMaxs[2] || bmaxs[2] <= vecMins[2] )
			return CMapHelper::IsIntersectingBox( vecMins, vecMaxs );

		return true;
	}
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
			CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );
			if ( m_pTemplate->LoadVMF( value, true ) )
			{
				m_pTemplate->GetMapWorld()->SetRenderColor( 134, 130, 0 );
				m_pTemplate->GetMapWorld()->SetModulationColor( Vector( 134 / 255.f, 130 / 255.f, 0 ) );
				m_pTemplate->GetMapWorld()->SetPreferredPickObject( GetParent() );
			}
		}
		PostUpdate( Notify_Changed );
	}
	else if ( !stricmp( key, "angles" ) && m_pTemplate->GetMapWorld() )
	{
		QAngle angle;
		Vector origin;
		DecompressMatrix( origin, angle );
		sscanf( value, "%f %f %f", &angle.x, &angle.y, &angle.z );
		ConstructMatrix( origin, angle );
		PostUpdate( Notify_Changed );
	}
	else if ( !stricmp( key, "origin" ) && m_pTemplate->GetMapWorld() )
	{
		QAngle angle;
		Vector origin;
		DecompressMatrix( origin, angle );
		sscanf( value, "%f %f %f", &origin.x, &origin.y, &origin.z );
		ConstructMatrix( origin, angle );
		PostUpdate( Notify_Changed );
	}
}

void CMapInstance::Render2DChildren( CRender2D* pRender, CMapClass* pEnt )
{
	CMapObjectList& children = pEnt->m_Children;
	for( CMapClass* pChild : children )
	{
		if ( pChild && pChild->IsVisible() && pChild->IsVisible2D() )
		{
			if ( CBaseTool* toolObject = pChild->GetToolObject( 0, false ) )
			{
				if ( toolObject->GetToolID() != TOOL_SWEPT_HULL )
					continue;
			}

			pChild->Render2D(pRender);
			Render2DChildren( pRender, pChild );
		}
	}
}

void CMapInstance::Render2D( CRender2D* pRender )
{
	if ( !m_pTemplate || !m_pTemplate->GetMapWorld() )
		return;

	CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );

	VMatrix localTransform;
	const bool inLocalTransform = pRender->IsInLocalTransformMode();
	if ( !inLocalTransform )
		pRender->BeginLocalTransfrom( m_matTransform );
	else
	{
		VMatrix newLocalTransform;
		pRender->GetLocalTranform( localTransform );
		pRender->EndLocalTransfrom();
		ConcatTransforms( localTransform.As3x4(), m_matTransform.As3x4(), newLocalTransform.As3x4() );
		pRender->BeginLocalTransfrom( newLocalTransform );
	}

	Render2DChildren( pRender, m_pTemplate->GetMapWorld() );

	if ( !inLocalTransform )
		pRender->EndLocalTransfrom();
	else
	{
		pRender->EndLocalTransfrom();
		pRender->BeginLocalTransfrom( localTransform );
	}

	pRender->PushRenderMode( RENDER_MODE_WIREFRAME );
	Vector mins, maxs;
	GetRender2DBox( mins, maxs );
	pRender->SetDrawColor( 255, 0, 255 );
	pRender->DrawBox( mins, maxs );
	pRender->PopRenderMode();
}

void CMapInstance::Render3DChildren( CRender3D* pRender, CUtlVector<CMapClass*>& deferred, CMapClass* pEnt )
{
	const EditorRenderMode_t renderMode = pRender->GetCurrentRenderMode();
	CMapObjectList& children = pEnt->m_Children;
	for( CMapClass* pChild : children )
	{
		if ( pChild && pChild->IsVisible() )
		{
			bool should_appear = true;
			if ( renderMode == RENDER_MODE_LIGHT_PREVIEW2 )
				should_appear &= pChild->ShouldAppearInLightingPreview();

			if ( renderMode == RENDER_MODE_LIGHT_PREVIEW_RAYTRACED )
				should_appear &= pChild->ShouldAppearInLightingPreview();

			if ( !should_appear )
				continue;

			if ( CBaseTool* toolObject = pChild->GetToolObject( 0, false ) )
			{
				if ( toolObject->GetToolID() != TOOL_SWEPT_HULL )
					continue;
			}

			if ( !pChild->ShouldRenderLast() )
				pChild->Render3D( pRender );
			else
			{
				deferred.AddToTail( pChild );
				continue;
			}
			Render3DChildren( pRender, deferred, pChild );
		}
	}
}

void CMapInstance::Render3DChildrenDeferred( CRender3D* pRender, CMapClass* pEnt )
{
	const EditorRenderMode_t renderMode = pRender->GetCurrentRenderMode();

	pEnt->Render3D( pRender );

	CMapObjectList& children = pEnt->m_Children;
	for( CMapClass* pChild : children )
	{
		if ( pChild && pChild->IsVisible() )
		{
			bool should_appear = true;
			if ( renderMode == RENDER_MODE_LIGHT_PREVIEW2 )
				should_appear &= pChild->ShouldAppearInLightingPreview();

			if ( renderMode == RENDER_MODE_LIGHT_PREVIEW_RAYTRACED )
				should_appear &= pChild->ShouldAppearInLightingPreview();

			if ( !should_appear )
				continue;

			pChild->Render3D( pRender );
			Render3DChildrenDeferred( pRender, pChild );
		}
	}
}

void CMapInstance::Render3D( CRender3D* pRender )
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
	{
		CAutoPushPop<CMapDoc*> guard( CMapDoc::m_pMapDoc, m_pTemplate );
		CAutoPushPop<bool> guard2( pRender->m_DeferRendering, false );

		VMatrix localTransform;
		const bool inLocalTransform = pRender->IsInLocalTransformMode();
		if ( !inLocalTransform )
			pRender->BeginLocalTransfrom( m_matTransform );
		else
		{
			pRender->GetLocalTranform( localTransform );
			pRender->EndLocalTransfrom();
			VMatrix newTransform = localTransform * m_matTransform;
			QAngle angle;
			const Vector origin = newTransform.GetTranslation();
			MatrixToAngles( newTransform, angle );
			newTransform.SetupMatrixOrgAngles( origin, angle );
			pRender->BeginLocalTransfrom( newTransform );
		}
		CUtlVector<CMapClass*> deferred;
		Render3DChildren( pRender, deferred, m_pTemplate->GetMapWorld() );

		for ( CMapClass* def : deferred )
			Render3DChildrenDeferred( pRender, def );

		if ( !inLocalTransform )
			pRender->EndLocalTransfrom();
		else
		{
			pRender->EndLocalTransfrom();
			pRender->BeginLocalTransfrom( localTransform );
		}
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
		AddShadowingTrianglesChildren( tri_list, m_pTemplate->GetMapWorld() );
}

void CMapInstance::AddShadowingTrianglesChildren( CUtlVector<Vector>& tri_list, CMapClass* pEnt )
{
	CMapObjectList& children = pEnt->m_Children;
	for( CMapClass* pChild : children )
	{
		if ( pChild && pChild->IsVisible() && pChild->ShouldAppearInLightingPreview() )
		{
			pChild->AddShadowingTriangles( tri_list );
			AddShadowingTrianglesChildren( tri_list, pChild );
		}
	}
}

void CMapInstance::RotateChild( const Vector& origin, const QAngle& angle, const Vector& translation, CMapClass* pEnt )
{
	CMapObjectList& children = pEnt->m_Children;
	for( CMapClass* pChild : children )
	{
		if ( pChild )
		{
			pChild->TransRotate( origin, angle );
			pChild->TransMove( translation );
			RotateChild( origin, angle, translation, pChild );
		}
	}
}

#pragma float_control(precise, on, push)
void CMapInstance::GetBounds( BoundBox CMapClass::* type, Vector& mins, Vector& maxs ) const
{
	if ( m_pTemplate && m_pTemplate->GetMapWorld() )
	{
		(m_pTemplate->GetMapWorld()->*type).GetBounds( mins, maxs );
		Vector box[8];
		PointsFromBox( mins, maxs, box );
		mins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		maxs.Init( FLT_MIN, FLT_MIN, FLT_MIN );
		for ( const Vector& point : box )
		{
			const Vector& transformed = m_matTransform.VMul4x3( point );
			mins = mins.Min( transformed );
			maxs = maxs.Max( transformed );
		}
		return;
	}
	Assert( 0 );
}

void CMapInstance::FixAngles( QAngle& angle )
{
	for ( int i = 0; i < 3; ++i )
	{
		if ( fabs( angle[i] ) < 0.001 )
			angle[i] = 0;
		else
			angle[i] = static_cast<int>( static_cast<double>( angle[i] ) * 100 + ( fsign( angle[i] ) * 0.5 ) ) / 100;
	}
}

void CMapInstance::DecompressMatrix( Vector& origin, QAngle& angle ) const
{
	origin = m_matTransform.GetTranslation();
	MatrixToAngles( m_matTransform, angle );
}

void CMapInstance::ConstructMatrix( const Vector& origin, const QAngle& angle )
{
	m_matTransform.SetupMatrixOrgAngles( origin, angle );
}
#pragma float_control(pop)