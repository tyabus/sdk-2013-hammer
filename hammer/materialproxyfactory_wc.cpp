//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "stdafx.h"
#include "tier1/strtools.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialproxyfactory.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "KeyValues.h"
#include "utldict.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


// ------------------------------------------------------------------------------------- //
// The material proxy factory for WC.
// ------------------------------------------------------------------------------------- //

class CMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	IMaterialProxy* CreateProxy( const char *proxyName ) override;
	void DeleteProxy( IMaterialProxy *pProxy ) override;

	using InstantiateProxyFn = IMaterialProxy* ( *)( );
	class CMaterialProxyFactoryRegister
	{
	public:
		CMaterialProxyFactoryRegister( const char* pProxyName, InstantiateProxyFn instFunc );
	};
private:
	CUtlDict<InstantiateProxyFn> m_proxies;
};
CMaterialProxyFactory g_MaterialProxyFactory;

#define REGISTER_PROXY(className, proxyName) \
	static IMaterialProxy* __Create##className##_proxy() { return new className; } \
	static CMaterialProxyFactory::CMaterialProxyFactoryRegister __##className##_proxy_register( proxyName, __Create##className##_proxy );


IMaterialProxy *CMaterialProxyFactory::CreateProxy( const char *proxyName )
{
	const int find = m_proxies.Find( proxyName );
	if ( m_proxies.IsValidIndex( find ) )
		return m_proxies[find]();
	return NULL;
}

void CMaterialProxyFactory::DeleteProxy( IMaterialProxy *pProxy )
{
	if ( pProxy )
		pProxy->Release();
}

CMaterialProxyFactory::CMaterialProxyFactoryRegister::CMaterialProxyFactoryRegister( const char* pProxyName, InstantiateProxyFn instFunc )
{
	g_MaterialProxyFactory.m_proxies.Insert( pProxyName, instFunc );
}

IMaterialProxyFactory* GetHammerMaterialProxyFactory()
{
	return &g_MaterialProxyFactory;
}


#pragma region Base

class CFloatInput
{
public:
	CFloatInput(): m_flValue( 0 ), m_pFloatVar( NULL ), m_FloatVecComp( 0 ) {}

	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues, const char* pKeyName, float flDefault = 0.0f )
	{
		m_pFloatVar = NULL;
		KeyValues* pSection = pKeyValues->FindKey( pKeyName );
		if ( pSection )
		{
			if ( pSection->GetDataType() == KeyValues::TYPE_STRING )
			{
				const char* pVarName = pSection->GetString();

				float flValue;
				const int nCount = sscanf( pVarName, "%f", &flValue );
				if ( nCount == 1 )
				{
					m_flValue = flValue;
					return true;
				}

				char pTemp[256];
				if ( strchr( pVarName, '[' ) )
				{
					Q_strncpy( pTemp, pVarName, 256 );
					char* pArray = strchr( pTemp, '[' );
					*pArray++ = 0;

					char* pIEnd;
					m_FloatVecComp = strtol( pArray, &pIEnd, 10 );

					pVarName = pTemp;
				}
				else
				{
					m_FloatVecComp = -1;
				}

				bool bFoundVar;
				m_pFloatVar = pMaterial->FindVar( pVarName, &bFoundVar, true );
				if ( !bFoundVar )
					return false;
			}
			else
			{
				m_flValue = pSection->GetFloat();
			}
		}
		else
		{
			m_flValue = flDefault;
		}
		return true;
	}

	float GetFloat() const
	{
		if (!m_pFloatVar)
			return m_flValue;

		if( m_FloatVecComp < 0 )
			return m_pFloatVar->GetFloatValue();

		const int iVecSize = m_pFloatVar->VectorSize();
		if ( m_FloatVecComp >= iVecSize )
			return 0;

		float v[4];
		m_pFloatVar->GetVecValue( v, iVecSize );
		return v[m_FloatVecComp];
	}

private:
	float m_flValue;
	IMaterialVar *m_pFloatVar;
	int	m_FloatVecComp;
};


class CResultProxy : public IMaterialProxy
{
public:
	CResultProxy() : m_pResult( 0 ), m_ResultVecComp( 0 ) {}
	virtual ~CResultProxy() = default;

	virtual bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		char const* pResult = pKeyValues->GetString( "resultVar" );
		if ( !pResult )
			return false;

		char pTemp[256];
		if ( strchr( pResult, '[' ) )
		{
			Q_strncpy( pTemp, pResult, 256 );
			char* pArray = strchr( pTemp, '[' );
			*pArray++    = 0;

			char* pIEnd;
			m_ResultVecComp = strtol( pArray, &pIEnd, 10 );

			pResult = pTemp;
		}
		else
		{
			m_ResultVecComp = -1;
		}

		bool foundVar;
		m_pResult = pMaterial->FindVar( pResult, &foundVar, true );
		return foundVar;
	}

	virtual void Release() { delete this; }
	virtual IMaterial *GetMaterial() { return m_pResult->GetOwningMaterial(); }

protected:
	void SetFloatResult( float result )
	{
		if ( m_pResult->GetType() == MATERIAL_VAR_TYPE_VECTOR )
		{
			if ( m_ResultVecComp >= 0 )
				m_pResult->SetVecComponentValue( result, m_ResultVecComp );
			else
			{
				float v[4];
				const int vecSize = m_pResult->VectorSize();
				for ( int i = 0; i < vecSize; ++i )
					v[i] = result;

				m_pResult->SetVecValue( v, vecSize );
			}
		}
		else
		{
			m_pResult->SetFloatValue( result );
		}
	}

	IMaterialVar* m_pResult;
	int m_ResultVecComp;
};

class CFunctionProxy : public CResultProxy
{
public:
	CFunctionProxy() : m_pSrc1( 0 ), m_pSrc2( 0 ) {}
	virtual ~CFunctionProxy() {}

	virtual bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		if ( !CResultProxy::Init( pMaterial, pKeyValues ) )
			return false;

		char const* pSrcVar1 = pKeyValues->GetString( "srcVar1" );
		if ( !pSrcVar1 )
			return false;

		bool foundVar;
		m_pSrc1 = pMaterial->FindVar( pSrcVar1, &foundVar, true );
		if ( !foundVar )
			return false;

		char const* pSrcVar2 = pKeyValues->GetString( "srcVar2" );
		if ( pSrcVar2 && ( *pSrcVar2 ) )
		{
			m_pSrc2 = pMaterial->FindVar( pSrcVar2, &foundVar, true );
			if ( !foundVar )
				return false;
		}
		else
		{
			m_pSrc2 = 0;
		}

		return true;
	}

protected:
	void ComputeResultType( MaterialVarType_t& resultType, int& vecSize )
	{
		resultType = m_pResult->GetType();
		if ( resultType == MATERIAL_VAR_TYPE_VECTOR )
		{
			if ( m_ResultVecComp >= 0 )
				resultType = MATERIAL_VAR_TYPE_FLOAT;
			vecSize = m_pResult->VectorSize();
		}
		else if ( resultType == MATERIAL_VAR_TYPE_UNDEFINED )
		{
			resultType = m_pSrc1->GetType();
			if ( resultType == MATERIAL_VAR_TYPE_VECTOR )
				vecSize = m_pSrc1->VectorSize();
			else if ( ( resultType == MATERIAL_VAR_TYPE_UNDEFINED ) && m_pSrc2 )
			{
				resultType = m_pSrc2->GetType();
				if ( resultType == MATERIAL_VAR_TYPE_VECTOR )
					vecSize = m_pSrc2->VectorSize();
			}
		}
	}

	IMaterialVar* m_pSrc1;
	IMaterialVar* m_pSrc2;
};

#pragma endregion

// ------------------------------------------------------------------------------------- //
// Specific material proxies.
// ------------------------------------------------------------------------------------- //

class CWorldDimsProxy : public IMaterialProxy
{
public:
	CWorldDimsProxy() : m_pMinsVar( NULL ), m_pMaxsVar( NULL ) {}
	virtual ~CWorldDimsProxy() = default;

	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		m_pMinsVar = pMaterial->FindVar( "$world_mins", NULL, false );
		m_pMaxsVar = pMaterial->FindVar( "$world_maxs", NULL, false );
		return true;
	}

	void OnBind( void* )
	{
		if ( m_pMinsVar && m_pMaxsVar )
		{
			const float mins[3] = { -500, -500, -500 };
			const float maxs[3] = { +500, +500, +500 };
			m_pMinsVar->SetVecValue( mins, 3 );
			m_pMaxsVar->SetVecValue( maxs, 3 );
		}
	}

	void Release() { delete this; }

	IMaterial* GetMaterial()
	{
		if ( m_pMinsVar && m_pMaxsVar )
			return m_pMinsVar->GetOwningMaterial();
		return NULL;
	}

private:
	IMaterialVar *m_pMinsVar;
	IMaterialVar *m_pMaxsVar;
};
REGISTER_PROXY( CWorldDimsProxy, "WorldDims" );


class CAddProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return CFunctionProxy::Init( pMaterial, pKeyValues ) && m_pSrc2;
	}

	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pSrc2 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector4D a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			m_pSrc2->GetVecValue( b.Base(), vecSize );
			Vector4DAdd( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

		case MATERIAL_VAR_TYPE_FLOAT:
			SetFloatResult( m_pSrc1->GetFloatValue() + m_pSrc2->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() + m_pSrc2->GetIntValue() );
			break;

		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CAddProxy, "Add" );

class CSubtractProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return CFunctionProxy::Init( pMaterial, pKeyValues ) && m_pSrc2;
	}

	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pSrc2 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector4D a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			m_pSrc2->GetVecValue( b.Base(), vecSize );
			Vector4DSubtract( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
			break;
		}

		case MATERIAL_VAR_TYPE_FLOAT:
			SetFloatResult( m_pSrc1->GetFloatValue() - m_pSrc2->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() - m_pSrc2->GetIntValue() );
			break;

		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CSubtractProxy, "Subtract" );

class CMultiplyProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return CFunctionProxy::Init( pMaterial, pKeyValues ) && m_pSrc2;
	}

	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pSrc2 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector4D a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			m_pSrc2->GetVecValue( b.Base(), vecSize );
			Vector4DMultiply( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
			break;
		}

		case MATERIAL_VAR_TYPE_FLOAT:
			SetFloatResult( m_pSrc1->GetFloatValue() * m_pSrc2->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() * m_pSrc2->GetIntValue() );
			break;

		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CMultiplyProxy, "Multiply" );

class CDivideProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return CFunctionProxy::Init( pMaterial, pKeyValues ) && m_pSrc2;
	}

	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pSrc2 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector4D a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			m_pSrc2->GetVecValue( b.Base(), vecSize );
			Vector4DDivide( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
			break;
		}

		case MATERIAL_VAR_TYPE_FLOAT:
			if (m_pSrc2->GetFloatValue() != 0)
				SetFloatResult( m_pSrc1->GetFloatValue() / m_pSrc2->GetFloatValue() );
			else
				SetFloatResult( m_pSrc1->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			if (m_pSrc2->GetIntValue() != 0)
				m_pResult->SetFloatValue( m_pSrc1->GetIntValue() / m_pSrc2->GetIntValue() );
			else
				m_pResult->SetFloatValue( m_pSrc1->GetIntValue() );
			break;

		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CDivideProxy, "Divide" );

class CClampProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		if ( !CFunctionProxy::Init( pMaterial, pKeyValues ) )
			return false;

		if ( !m_Min.Init( pMaterial, pKeyValues, "min", 0 ) )
			return false;

		if ( !m_Max.Init( pMaterial, pKeyValues, "max", 1 ) )
			return false;

		return true;
	}

	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		float flMin = m_Min.GetFloat();
		float flMax = m_Max.GetFloat();

		if ( flMin > flMax )
			V_swap( flMin, flMax );

		switch ( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
			{
				Vector4D a;
				m_pSrc1->GetVecValue( a.Base(), vecSize );
				for ( int i = 0; i < vecSize; ++i )
				{
					if ( a[i] < flMin )
						a[i] = flMin;
					else if ( a[i] > flMax )
						a[i] = flMax;
				}
				m_pResult->SetVecValue( a.Base(), vecSize );
				break;
			}

		case MATERIAL_VAR_TYPE_FLOAT:
			{
				float src = m_pSrc1->GetFloatValue();
				if ( src < flMin )
					src = flMin;
				else if ( src > flMax )
					src = flMax;
				SetFloatResult( src );
				break;
			}

		case MATERIAL_VAR_TYPE_INT:
			{
				int src = m_pSrc1->GetIntValue();
				if ( src < flMin )
					src = flMin;
				else if ( src > flMax )
					src = flMax;
				m_pResult->SetIntValue( src );
				break;
			}
		default:
			Assert( 0 );
		}
	}

private:
	CFloatInput m_Min;
	CFloatInput m_Max;
};
REGISTER_PROXY( CClampProxy, "Clamp" );

class CSineProxy : public CResultProxy
{
public:
	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		if ( !CResultProxy::Init( pMaterial, pKeyValues ) )
			return false;

		if ( !m_SinePeriod.Init( pMaterial, pKeyValues, "sinePeriod", 1.0f ) )
			return false;
		if ( !m_SineMax.Init( pMaterial, pKeyValues, "sineMax", 1.0f ) )
			return false;
		if ( !m_SineMin.Init( pMaterial, pKeyValues, "sineMin", 0.0f ) )
			return false;
		if ( !m_SineTimeOffset.Init( pMaterial, pKeyValues, "timeOffset", 0.0f ) )
			return false;

		return true;
	}

	void OnBind( void* )
	{
		Assert( m_pResult );

		const float flSineMin = m_SineMin.GetFloat();
		float flSinePeriod = m_SinePeriod.GetFloat();
		if ( flSinePeriod == 0 )
			flSinePeriod = 1;

		float flValue = ( sin( 2.0f * M_PI * ( Plat_FloatTime() - m_SineTimeOffset.GetFloat() ) / flSinePeriod ) * 0.5f ) + 0.5f;
		flValue = ( m_SineMax.GetFloat() - flSineMin ) * flValue + flSineMin;

		SetFloatResult( flValue );
	}

private:
	CFloatInput m_SinePeriod;
	CFloatInput m_SineMax;
	CFloatInput m_SineMin;
	CFloatInput m_SineTimeOffset;
};
REGISTER_PROXY( CSineProxy, "Sine" );

class CEqualsProxy : public CFunctionProxy
{
public:
	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector4D a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			m_pResult->SetVecValue( a.Base(), vecSize );
			break;
		}

		case MATERIAL_VAR_TYPE_FLOAT:
			SetFloatResult( m_pSrc1->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
			break;
		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CEqualsProxy, "Equals" );

class CFracProxy : public CFunctionProxy
{
public:
	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch ( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
			{
				Vector4D a;
				m_pSrc1->GetVecValue( a.Base(), vecSize );
				a[0] -= static_cast<float>( static_cast<int>( a[0] ) );
				a[1] -= static_cast<float>( static_cast<int>( a[1] ) );
				a[2] -= static_cast<float>( static_cast<int>( a[2] ) );
				a[3] -= static_cast<float>( static_cast<int>( a[3] ) );
				m_pResult->SetVecValue( a.Base(), vecSize );
				break;
			}

		case MATERIAL_VAR_TYPE_FLOAT:
			{
				float a = m_pSrc1->GetFloatValue();
				a -= static_cast<int>( a );
				SetFloatResult( a );
				break;
			}

		case MATERIAL_VAR_TYPE_INT:
			m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
			break;

		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CFracProxy, "Frac" );

class CIntProxy : public CFunctionProxy
{
public:
	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch ( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
			{
				Vector4D a;
				m_pSrc1->GetVecValue( a.Base(), vecSize );
				a[0] = static_cast<float>( static_cast<int>( a[0] ) );
				a[1] = static_cast<float>( static_cast<int>( a[1] ) );
				a[2] = static_cast<float>( static_cast<int>( a[2] ) );
				a[3] = static_cast<float>( static_cast<int>( a[3] ) );
				m_pResult->SetVecValue( a.Base(), vecSize );
				break;
			}

		case MATERIAL_VAR_TYPE_FLOAT:
			{
				const float a = static_cast<float>( static_cast<int>( m_pSrc1->GetFloatValue() ) );
				SetFloatResult( a );
				break;
			}

		case MATERIAL_VAR_TYPE_INT:
			// don't do anything besides assignment!
			m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
			break;

		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CIntProxy, "Int" );

class CLinearRampProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		if ( !CResultProxy::Init( pMaterial, pKeyValues ) )
			return false;

		if ( !m_Rate.Init( pMaterial, pKeyValues, "rate", 1 ) )
			return false;

		if ( !m_InitialValue.Init( pMaterial, pKeyValues, "initialValue", 0 ) )
			return false;

		return true;
	}

	void OnBind( void* )
	{
		Assert( m_pResult );

		// get a value in [0,1]
		const float flValue = m_Rate.GetFloat() * Plat_FloatTime() + m_InitialValue.GetFloat();
		SetFloatResult( flValue );
	}

private:
	CFloatInput m_Rate;
	CFloatInput m_InitialValue;
};
REGISTER_PROXY( CLinearRampProxy, "LinearRamp" );

static CUniformRandomStream random;
class CUniformNoiseProxy : public CResultProxy
{
public:
	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		if ( !CResultProxy::Init( pMaterial, pKeyValues ) )
			return false;

		if ( !m_flMinVal.Init( pMaterial, pKeyValues, "minVal", 0 ) )
			return false;

		if ( !m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", 1 ) )
			return false;

		static bool first = true;
		if ( first )
		{
			random.SetSeed( time( NULL ) % 32768 );
			first = false;
		}

		return true;
	}

	void OnBind( void* )
	{
		SetFloatResult( random.RandomFloat( m_flMinVal.GetFloat(), m_flMaxVal.GetFloat() ) );
	}

private:
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};
REGISTER_PROXY( CUniformNoiseProxy, "UniformNoise" );

static CGaussianRandomStream randomgaussian;
static CUniformRandomStream randomgaussianbase;
class CGaussianNoiseProxy : public CResultProxy
{
public:
	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		if ( !CResultProxy::Init( pMaterial, pKeyValues ) )
			return false;

		if ( !m_Mean.Init( pMaterial, pKeyValues, "mean", 0.0f ) )
			return false;

		if ( !m_StdDev.Init( pMaterial, pKeyValues, "halfwidth", 1.0f ) )
			return false;

		if ( !m_flMinVal.Init( pMaterial, pKeyValues, "minVal", -FLT_MAX ) )
			return false;

		if ( !m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", FLT_MAX ) )
			return false;

		static bool first = true;
		if ( first )
		{
			randomgaussianbase.SetSeed( time( NULL ) % 32768 );
			randomgaussian.AttachToStream( &randomgaussianbase );
			first = false;
		}

		return true;
	}

	void OnBind( void* )
	{
		float flVal = randomgaussian.RandomFloat( m_Mean.GetFloat(), m_StdDev.GetFloat() );
		float flMaxVal = m_flMaxVal.GetFloat();
		float flMinVal = m_flMinVal.GetFloat();

		if ( flMinVal > flMaxVal )
			V_swap( flMinVal, flMaxVal );

		if ( flVal < flMinVal )
			flVal = flMinVal;
		else if ( flVal > flMaxVal )
			flVal = flMaxVal;

		SetFloatResult( flVal );
	}

private:
	CFloatInput m_Mean;
	CFloatInput m_StdDev;
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};
REGISTER_PROXY( CGaussianNoiseProxy, "GaussianNoise" );

class CExponentialProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		if ( !CFunctionProxy::Init( pMaterial, pKeyValues ) )
			return false;

		if ( !m_Scale.Init( pMaterial, pKeyValues, "scale", 1.0f ) )
			return false;

		if ( !m_Offset.Init( pMaterial, pKeyValues, "offset", 0.0f ) )
			return false;

		if ( !m_flMinVal.Init( pMaterial, pKeyValues, "minVal", -FLT_MAX ) )
			return false;

		if ( !m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", FLT_MAX ) )
			return false;

		return true;
	}

	void OnBind( void* )
	{
		float flVal = m_Scale.GetFloat() * exp( m_pSrc1->GetFloatValue() + m_Offset.GetFloat() );

		float flMaxVal = m_flMaxVal.GetFloat();
		float flMinVal = m_flMinVal.GetFloat();

		if ( flMinVal > flMaxVal )
			V_swap( flMinVal, flMaxVal );

		if ( flVal < flMinVal )
			flVal = flMinVal;
		else if ( flVal > flMaxVal )
			flVal = flMaxVal;

		SetFloatResult( flVal );
	}

private:
	CFloatInput	m_Scale;
	CFloatInput	m_Offset;
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};
REGISTER_PROXY( CExponentialProxy, "Exponential" );

class CAbsProxy : public CFunctionProxy
{
public:
	void OnBind( void* )
	{
		SetFloatResult( fabs( m_pSrc1->GetFloatValue() ) );
	}
};
REGISTER_PROXY( CAbsProxy, "Abs" );

class CEmptyProxy : public IMaterialProxy
{
public:
	CEmptyProxy() = default;
	~CEmptyProxy() = default;
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) { return true; }
	void OnBind( void* ) {}
	void Release() { delete this; }
	IMaterial *GetMaterial() { return NULL; }
};
REGISTER_PROXY( CEmptyProxy, "Empty" );

class CLessOrEqualProxy : public CFunctionProxy
{
public:
	CLessOrEqualProxy() : m_pLessVar( NULL ), m_pGreaterVar( NULL ) {}
	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		char const* pLessEqualVar = pKeyValues->GetString( "lessEqualVar" );
		if ( !pLessEqualVar )
			return false;

		bool foundVar;
		m_pLessVar = pMaterial->FindVar( pLessEqualVar, &foundVar, true );
		if ( !foundVar )
			return false;

		char const* pGreaterVar = pKeyValues->GetString( "greaterVar" );
		if ( !pGreaterVar )
			return false;

		m_pGreaterVar = pMaterial->FindVar( pGreaterVar, &foundVar, true );
		if ( !foundVar )
			return false;

		return CFunctionProxy::Init( pMaterial, pKeyValues ) && m_pSrc2;
	}

	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pSrc2 && m_pLessVar && m_pGreaterVar && m_pResult );

		IMaterialVar* pSourceVar;
		if ( m_pSrc1->GetFloatValue() <= m_pSrc2->GetFloatValue() )
			pSourceVar = m_pLessVar;
		else
			pSourceVar = m_pGreaterVar;

		int vecSize = 0;
		MaterialVarType_t resultType = m_pResult->GetType();
		if ( resultType == MATERIAL_VAR_TYPE_VECTOR )
		{
			if ( m_ResultVecComp >= 0 )
				resultType = MATERIAL_VAR_TYPE_FLOAT;
			vecSize = m_pResult->VectorSize();
		}
		else if ( resultType == MATERIAL_VAR_TYPE_UNDEFINED )
		{
			resultType = pSourceVar->GetType();
			if ( resultType == MATERIAL_VAR_TYPE_VECTOR )
				vecSize = pSourceVar->VectorSize();
		}

		switch ( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
			{
				Vector4D src;
				pSourceVar->GetVecValue( src.Base(), vecSize );
				m_pResult->SetVecValue( src.Base(), vecSize );
				break;
			}

		case MATERIAL_VAR_TYPE_FLOAT:
			SetFloatResult( pSourceVar->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			m_pResult->SetFloatValue( pSourceVar->GetIntValue() );
			break;
		default:
			Assert( 0 );
		}
	}

private:
	IMaterialVar *m_pLessVar;
	IMaterialVar *m_pGreaterVar;
};
REGISTER_PROXY( CLessOrEqualProxy, "LessOrEqual" );

class CWrapMinMaxProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial* pMaterial, KeyValues* pKeyValues )
	{
		if ( !CFunctionProxy::Init( pMaterial, pKeyValues ) )
			return false;

		if ( !m_flMinVal.Init( pMaterial, pKeyValues, "minVal", 0 ) )
			return false;

		if ( !m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", 1 ) )
			return false;

		return true;
	}

	void OnBind( void* )
	{
		Assert( m_pSrc1 && m_pResult );

		if ( m_flMaxVal.GetFloat() <= m_flMinVal.GetFloat() ) // Bad input, just return the min
			SetFloatResult( m_flMinVal.GetFloat() );
		else
		{
			float flResult = ( m_pSrc1->GetFloatValue() - m_flMinVal.GetFloat() ) / ( m_flMaxVal.GetFloat() - m_flMinVal.GetFloat() );

			if ( flResult >= 0.0f )
				flResult -= static_cast<float>( static_cast<int>( flResult ) );
			else // Negative
				flResult -= static_cast<float>( static_cast<int>( flResult ) - 1 );

			flResult *= m_flMaxVal.GetFloat() - m_flMinVal.GetFloat();
			flResult += m_flMinVal.GetFloat();

			SetFloatResult( flResult );
		}
	}

private:
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};
REGISTER_PROXY( CWrapMinMaxProxy, "WrapMinMax" );

class CSelectFirstIfNonZeroProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return CFunctionProxy::Init( pMaterial, pKeyValues ) && m_pSrc2;
	}

	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pSrc1 && m_pSrc2 && m_pResult );

		MaterialVarType_t resultType;
		int vecSize;
		ComputeResultType( resultType, vecSize );

		switch ( resultType )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
			{
				Vector4D a( 0, 0, 0, 0 ), b( 0, 0, 0, 0 );
				m_pSrc1->GetVecValue( a.Base(), vecSize );
				m_pSrc2->GetVecValue( b.Base(), vecSize );

				if ( !a.IsZero() )
					m_pResult->SetVecValue( a.Base(), vecSize );
				else
					m_pResult->SetVecValue( b.Base(), vecSize );
				break;
			}

		case MATERIAL_VAR_TYPE_FLOAT:
			if ( m_pSrc1->GetFloatValue() )
				SetFloatResult( m_pSrc1->GetFloatValue() );
			else
				SetFloatResult( m_pSrc2->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			if ( m_pSrc1->GetIntValue() )
				m_pResult->SetFloatValue( m_pSrc1->GetIntValue() );
			else
				m_pResult->SetFloatValue( m_pSrc2->GetIntValue() );
			break;
		default:
			Assert( 0 );
		}
	}
};
REGISTER_PROXY( CSelectFirstIfNonZeroProxy, "SelectFirstIfNonZero" );