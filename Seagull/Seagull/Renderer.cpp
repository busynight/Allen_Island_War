//--------------------------------------------------------------------------------------
// File: Renderer.cpp
//
// This is a simple Windows Store app showing use of DirectXTK
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Renderer.h"

#include "DDSTextureLoader.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

class MyEffectFactory : public EffectFactory
{
public:
    MyEffectFactory( _In_ ID3D11Device* device ) : EffectFactory( device ) {}

    virtual void CreateTexture( _In_z_ const WCHAR* name, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView ) override
    {
        WCHAR path[MAX_PATH] = {0};

        wcscpy_s( path, L"assets\\" );
        wcscat_s( path, name );

        EffectFactory::CreateTexture( path, deviceContext, textureView );
    }
};

Renderer::Renderer()
{
}

void Renderer::CreateDeviceResources()
{
	Direct3DBase::CreateDeviceResources();

    // Create DirectXTK objects
    auto device = m_d3dDevice.Get();
    m_states.reset( new CommonStates( device ) );
    m_fxFactory.reset( new MyEffectFactory( device ) );

    auto context = m_d3dContext.Get();
    m_sprites.reset( new SpriteBatch( context ) );
    m_batch.reset( new PrimitiveBatch<VertexPositionColor>( context ) );

    m_batchEffect.reset( new BasicEffect( device ) );
    m_batchEffect->SetVertexColorEnabled(true);

		void const* shaderByteCode;
        size_t byteCodeLength;

        m_batchEffect->GetVertexShaderBytecode( &shaderByteCode, &byteCodeLength );
    
      

        DX::ThrowIfFailed(
            device->CreateInputLayout( VertexPositionColor::InputElements,
                                        VertexPositionColor::InputElementCount,
                                        shaderByteCode, byteCodeLength,
                                        m_batchInputLayout.ReleaseAndGetAddressOf() ) 
                         );
    

    m_font.reset( new SpriteFont( device, L"assets\\italic.spritefont" ) );

    //m_shape = GeometricPrimitive::CreateTeapot( context, 4.f, 8 );

    // SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    //m_model = Model::CreateFromSDKMESH( device, L"assets\\tiny.sdkmesh", *m_fxFactory );

    // Load textures
   /* DX::ThrowIfFailed(
        CreateDDSTextureFromFile( device, L"assets\\seafloor.dds", nullptr, m_texture1.ReleaseAndGetAddressOf() )
                     );*/

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile( device, L"assets\\seafloor.dds", nullptr, m_texture2.ReleaseAndGetAddressOf() )
                     );
}

void Renderer::CreateWindowSizeDependentResources()
{
	Direct3DBase::CreateWindowSizeDependentResources();

	float aspectRatio = m_windowBounds.Width / m_windowBounds.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// Note that the m_orientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

    XMMATRIX projection = 
			XMMatrixMultiply(
				XMMatrixPerspectiveFovRH(
					fovAngleY,
					aspectRatio,
					0.01f,
					100.0f
					),
				XMLoadFloat4x4(&m_orientationTransform3D)
			);

    m_batchEffect->SetProjection( projection );

	XMStoreFloat4x4( &m_projection, projection );
}

void Renderer::Update(float timeTotal, float timeDelta)
{
    UNREFERENCED_PARAMETER( timeDelta );

	XMVECTOR eye = XMVectorSet(0.0f, 0.7f, 1.5f, 0.0f);
	XMVECTOR at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtRH(eye, at, up);
    XMMATRIX world = XMMatrixRotationY(timeTotal * XM_PIDIV4);

    m_batchEffect->SetView( view );
    m_batchEffect->SetWorld( XMMatrixIdentity() );

	XMStoreFloat4x4(&m_view, view);
	XMStoreFloat4x4(&m_world, world);
}

void Renderer::DrawGrid( FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color )
{
    auto context = m_d3dContext.Get();
    m_batchEffect->Apply( context );

    context->IASetInputLayout( m_batchInputLayout.Get() );

    m_batch->Begin();

    xdivs = std::max<size_t>( 1, xdivs );
    ydivs = std::max<size_t>( 1, ydivs );

    for( size_t i = 0; i <= xdivs; ++i )
    {
        float fPercent = float(i) / float(xdivs);
        fPercent = ( fPercent * 2.0f ) - 1.0f;
        XMVECTOR vScale = XMVectorScale( xAxis, fPercent );
        vScale = XMVectorAdd( vScale, origin );

        VertexPositionColor v1( XMVectorSubtract( vScale, yAxis ), color );
        VertexPositionColor v2( XMVectorAdd( vScale, yAxis ), color );
        m_batch->DrawLine( v1, v2 );
    }

    for( size_t i = 0; i <= ydivs; i++ )
    {
        FLOAT fPercent = float(i) / float(ydivs);
        fPercent = ( fPercent * 2.0f ) - 1.0f;
        XMVECTOR vScale = XMVectorScale( yAxis, fPercent );
        vScale = XMVectorAdd( vScale, origin );

        VertexPositionColor v1( XMVectorSubtract( vScale, xAxis ), color );
        VertexPositionColor v2( XMVectorAdd( vScale, xAxis ), color );
        m_batch->DrawLine( v1, v2 );
    }

    m_batch->End();
}

void Renderer::Render()
{
	m_d3dContext->ClearRenderTargetView(
		m_renderTargetView.Get(),
		Colors::MidnightBlue
		);

	m_d3dContext->ClearDepthStencilView(
		m_depthStencilView.Get(),
		D3D11_CLEAR_DEPTH,
		1.0f,
		0
		);

	m_d3dContext->OMSetRenderTargets(
		1,
		m_renderTargetView.GetAddressOf(),
		m_depthStencilView.Get()
		);

 //   // Draw procedurally generated dynamic grid
    const XMVECTORF32 xaxis = { 20.f, 0.f, 0.f };
    const XMVECTORF32 yaxis = { 0.f, 0.f, 20.f };
    DrawGrid( xaxis, yaxis, g_XMZero, 20, 20, Colors::Gray );

    // Draw sprite
    m_sprites->Begin();
	m_sprites->Draw( m_texture2.Get(), XMFLOAT2(100.7f,100.6f), nullptr, Colors::White);

    //m_font->DrawString( m_sprites.get(), L"DirectXTK Simple Sample", XMFLOAT2( 100, 10 ), Colors::Yellow );
    m_sprites->End();

 //   // Draw 3D object
    /*XMMATRIX world = XMLoadFloat4x4( &m_world );
    XMMATRIX view = XMLoadFloat4x4( &m_view );
    XMMATRIX projection = XMLoadFloat4x4( &m_projection );

    XMMATRIX local = XMMatrixMultiply( world, XMMatrixTranslation( -2.f, -2.f, -4.f ) );
    m_shape->Draw( local, view, projection, Colors::White, m_texture1.Get() );

    XMVECTOR qid = XMQuaternionIdentity();
    const XMVECTORF32 scale = { 0.01f, 0.01f, 0.01f};
    const XMVECTORF32 translate = { 3.f, -2.f, -4.f };
    XMVECTOR rotate = XMQuaternionRotationRollPitchYaw( 0, XM_PI/2.f, -XM_PI/2.f );
    local = XMMatrixMultiply( world, XMMatrixTransformation( g_XMZero, qid, scale, g_XMZero, rotate, translate ) );
    m_model->Draw( m_d3dContext.Get(), *m_states, local, view, projection );*/
}
