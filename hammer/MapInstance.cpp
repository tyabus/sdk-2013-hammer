#include "stdafx.h"
#include "MapInstance.h"
#include "mapentity.h"
#include "mapdoc.h"
#include "mapworld.h"
#include "render2d.h"
#include "render3dms.h"
#include "toolinterface.h"
#include "mapview2d.h"

#include "smartptr.h"
#include "fmtstr.h"
#include "chunkfile.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

IMPLEMENT_MAPCLASS( CMapInstance );

CMapClass* CMapInstance::Create( CHelperInfo* pHelperInfo, CMapEntity* pParent )
{
	return new CMapInstance( pParent );
}

CMapInstance::CMapInstance() : m_pTemplate( NULL )
{
	m_matTransform.Identity();
}

CMapInstance::CMapInstance( CMapEntity* pParent ) : m_pTemplate( NULL )
{
	m_matTransform.Identity();
	m_strCurrentVMF = pParent->GetKeyValue( "file" );
	LoadVMF( pParent );
}

CMapInstance::~CMapInstance()
{
	delete m_pTemplate;
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
	m_strPreviousVMF = pObject->m_strPreviousVMF;
	m_matTransform = pObject->m_matTransform;

	return this;
}

void CMapInstance::UpdateDependencies( CMapWorld* pWorld, CMapClass* pObject )
{
	if ( m_strCurrentVMF != m_strPreviousVMF )
		LoadVMF();
}

void CMapInstance::SetParent( CMapAtom* pParent )
{
	CMapHelper::SetParent( pParent );
	if ( m_pTemplate )
		m_pTemplate->SetPreferredPickObject( GetParent() );
}

SelectionState_t CMapInstance::SetSelectionState( SelectionState_t eSelectionState )
{
	const SelectionState_t old = CMapHelper::SetSelectionState( eSelectionState );
	if ( m_pTemplate )
		m_pTemplate->SetSelectionState( eSelectionState == SELECT_NONE ? SELECT_NONE : SELECT_NORMAL );
	return old;
}

void CMapInstance::SetOrigin( Vector& pfOrigin )
{
	CMapHelper::SetOrigin( pfOrigin );
}

void CMapInstance::SetCullBoxFromFaceList( CMapFaceList* pFaces )
{
	if ( m_pTemplate )
		m_pTemplate->SetCullBoxFromFaceList( pFaces );
	else
		CMapHelper::SetCullBoxFromFaceList( pFaces );
}

void CMapInstance::CalcBounds( BOOL bFullUpdate )
{
	if ( m_pTemplate )
		m_pTemplate->CalcBounds( bFullUpdate );

	m_Render2DBox.bmins = m_CullBox.bmins = m_Origin - Vector( 8 );
	m_Render2DBox.bmaxs = m_CullBox.bmaxs = m_Origin + Vector( 8 );
}

void CMapInstance::GetCullBox( Vector& mins, Vector& maxs )
{
	if ( m_pTemplate )
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
	if ( m_pTemplate )
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
	if ( m_pTemplate )
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
	if ( m_pTemplate )
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

	while ( angle[YAW] < 0 )
	{
		angle[YAW] += 360;
	}

	if ( CMapEntity *pEntity = dynamic_cast<CMapEntity*>( m_pParent ) )
	{
		char szValue[80];
		sprintf(szValue, "%g %g %g", angle[0], angle[1], angle[2]);
		pEntity->NotifyChildKeyChanged( this, "angles", szValue );
	}
}
#pragma float_control(pop)

bool CMapInstance::PostloadVisGroups( bool bIsLoading )
{
	if ( m_pTemplate )
	{
		m_pTemplate->PostloadVisGroups();
	}

	return CMapHelper::PostloadVisGroups( bIsLoading );
}

bool CMapInstance::HitTest2D( CMapView2D* pView, const Vector2D& point, HitInfo_t& HitData )
{
	Vector world;
	pView->ClientToWorld( world, point );
	Vector2D transformed;
	pView->WorldToClient( transformed, m_matTransform.InverseTR().VMul4x3( world ) );
	if ( m_pTemplate && m_pTemplate->HitTest2D( pView, transformed, HitData ) )
		return true;
	return CMapHelper::HitTest2D( pView, point, HitData );
}

bool CMapInstance::IsCulledByCordon( const Vector& vecMins, const Vector& vecMaxs )
{
	return !IsIntersectingBox( vecMins, vecMaxs );
}

bool CMapInstance::IsInsideBox( Vector const& pfMins, Vector const& pfMaxs ) const
{
	if ( m_pTemplate )
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
	if ( m_pTemplate )
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
		m_strPreviousVMF = m_strCurrentVMF;
		m_strCurrentVMF.Set( value );
		LoadVMF();
		PostUpdate( Notify_Changed );
	}
	else if ( !stricmp( key, "angles" ) )
	{
		QAngle angle;
		Vector origin;
		DecompressMatrix( origin, angle );
		sscanf( value, "%f %f %f", &angle.x, &angle.y, &angle.z );
		ConstructMatrix( origin, angle );
		PostUpdate( Notify_Changed );
	}
	else if ( !stricmp( key, "origin" ) )
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
	if ( !m_pTemplate )
		return;

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

	Render2DChildren( pRender, m_pTemplate );

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
	for ( CMapClass* pChild : children )
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
	if ( m_pTemplate )
	{
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
		Render3DChildren( pRender, deferred, m_pTemplate );

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
	if ( m_pTemplate )
	{
		return m_pTemplate->RenderPreload( pRender, bNewContext );
	}
	else
		return CMapHelper::RenderPreload( pRender, bNewContext );
}

void CMapInstance::AddShadowingTriangles( CUtlVector<Vector>& tri_list )
{
	if ( m_pTemplate )
		AddShadowingTrianglesChildren( tri_list, m_pTemplate );
}

void CMapInstance::LoadVMF( CMapClass* pParent )
{
	if ( m_pTemplate )
	{
		delete m_pTemplate;
		m_pTemplate = NULL;
	}

	if ( !m_strCurrentVMF.IsEmpty() )
	{
		CMapWorld* world = GetWorldObject( pParent ? pParent : GetParent() );
		Assert( world );
		char parentDir[MAX_PATH];
		V_ExtractFilePath( world->GetVMFPath(), parentDir, MAX_PATH );
		const CFmtStr instancePath( "%s" CORRECT_PATH_SEPARATOR_S "%s", parentDir, m_strCurrentVMF.Get() );
		if ( g_pFullFileSystem->FileExists( instancePath ) && LoadVMFInternal( instancePath ) )
		{
			m_pTemplate->SetRenderColor( 134, 130, 0 );
			m_pTemplate->SetModulationColor( Vector( 134 / 255.f, 130 / 255.f, 0 ) );
			m_pTemplate->SetPreferredPickObject( pParent ? pParent : GetParent() );
		}
	}
}

bool CMapInstance::LoadVMFInternal( const char* pVMFPath )
{
	if ( !m_pTemplate )
		m_pTemplate = new CMapWorld;
	m_pTemplate->SetVMFPath( pVMFPath );

	CChunkFile file;
	ChunkFileResult_t eResult = file.Open( pVMFPath, ChunkFile_Read );
	if ( eResult == ChunkFile_Ok )
	{
		ChunkFileResult_t (*LoadWorldCallback)(CChunkFile*, CMapInstance*) = []( CChunkFile *pFile, CMapInstance *pDoc )
		{
			return pDoc->m_pTemplate->LoadVMF( pFile );
		};

		ChunkFileResult_t (*LoadEntityCallback)(CChunkFile*, CMapInstance*) = [](CChunkFile *pFile, CMapInstance *pDoc)
		{
			CMapEntity* pEntity = new CMapEntity;
			if ( pEntity->LoadVMF( pFile ) == ChunkFile_Ok )
				pDoc->m_pTemplate->AddChild( pEntity );
			return ChunkFile_Ok;
		};

		CChunkHandlerMap handlers;
		handlers.AddHandler( "world", LoadWorldCallback, this );
		handlers.AddHandler( "entity", LoadEntityCallback, this );
		handlers.SetErrorHandler( []( CChunkFile*, const char*, void* ) { return false; }, NULL );

		file.PushHandlers( &handlers );

		while ( eResult == ChunkFile_Ok )
			eResult = file.ReadChunk();

		if ( eResult == ChunkFile_EOF )
			eResult = ChunkFile_Ok;

		file.PopHandlers();
	}

	if ( eResult == ChunkFile_Ok )
		m_pTemplate->PostloadWorld();
	else
	{
		delete m_pTemplate;
		m_pTemplate = NULL;
	}

	return eResult == ChunkFile_Ok;
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

#pragma float_control(precise, on, push)
void CMapInstance::GetBounds( BoundBox CMapClass::* type, Vector& mins, Vector& maxs ) const
{
	if ( m_pTemplate )
	{
		(m_pTemplate->*type).GetBounds( mins, maxs );
		Vector box[8];
		PointsFromBox( mins, maxs, box );
		for ( int i = 0; i < 8; ++i )
		{
			const Vector& transformed = m_matTransform.VMul4x3( box[i] );
			for ( int j = 0; j < 3; ++j )
			{
				if ( i == 0 || transformed[j] < mins[j] )
				{
					mins[j] = transformed[j];
				}
				if ( i == 0 || transformed[j] > maxs[j] )
				{
					maxs[j] = transformed[j];
				}
			}
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