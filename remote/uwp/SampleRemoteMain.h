//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "Common/DeviceResources.h"

#include "Content/PerceptionDeviceHandler.h"
#include "Content/QRCodeRenderer.h"
#include "Content/SpatialInputHandler.h"
#include "Content/SpatialInputRenderer.h"
#include "Content/SpatialSurfaceMeshRenderer.h"
#include "Content/SpinningCubeRenderer.h"

#include <memory>

#include <winrt/Microsoft.Holographic.AppRemoting.h>

#define INITIAL_WINDOW_WIDTH 1280
#define INITIAL_WINDOW_HEIGHT 720

#define TITLE_TEXT L"Remoting Host Sample"
#define TITLE_SEPARATOR L" | "
#define TITLE_CONNECT_TEXT L"Press Space To Connect"
#define TITLE_DISCONNECT_TEXT L"Press D to Disconnect"
#define TITLE_ENABLE_PREVIEW_TEXT L"Preview Disabled (press P to enable)"
#define TITLE_DISABLE_PREVIEW_TEXT L"Preview Enabled (press P to disable)"

// #define ENABLE_CUSTOM_DATA_CHANNEL_SAMPLE

class SampleRemoteMain : public std::enable_shared_from_this<SampleRemoteMain>, public DXHelper::IDeviceNotify
{
public:
    struct IWindow
    {
        virtual winrt::com_ptr<IDXGISwapChain1>
            CreateSwapChain(const winrt::com_ptr<ID3D11Device1>& device, const DXGI_SWAP_CHAIN_DESC1* desc) = 0;

        virtual winrt::Windows::Graphics::Holographic::HolographicSpace CreateHolographicSpace() = 0;

        virtual winrt::Windows::UI::Input::Spatial::SpatialInteractionManager CreateInteractionManager() = 0;

        virtual void SetWindowTitle(std::wstring title) = 0;
    };

public:
    SampleRemoteMain(std::weak_ptr<IWindow> window);
    ~SampleRemoteMain();

    // Creates a HolographicFrame and updates the content.
    winrt::Windows::Graphics::Holographic::HolographicFrame Update();

    // Renders the current frame to each holographic camera and presents it.
    void Render(winrt::Windows::Graphics::Holographic::HolographicFrame holographicFrame);

    const std::shared_ptr<DXHelper::DeviceResources>& GetDeviceResources()
    {
        return m_deviceResources;
    }

    // Initialize SampleRemoteMain for remote rendering targeting a HolographicRemotingPlayer.
    void
        ConfigureRemoting(bool listen, const std::wstring& hostname, uint16_t port, uint16_t transportPort = 0, bool ephemeralPort = false);

    // Initialize SampleRemoteMain for local rendering targeting HoloLens or Windows Mixed Reality headsets.
    void InitializeStandalone();

    // Responds to key presses.
    void OnKeyPress(char key);

    // Responds to window changing its size.
    void OnResize(int width, int height);

    // Responds to speech recognition results.
    void OnRecognizedSpeech(const winrt::hstring& recognizedText);

    // IDeviceNotify methods
    virtual void OnDeviceLost();
    virtual void OnDeviceRestored();

    // Initializes the RemoteContext and starts connecting or listening to the currently set network address
    void InitializeRemoteContextAndConnectOrListen();

private:
    // Initializes the HolographicSpace and creates graphics device dependent resources
    void CreateHolographicSpaceAndDeviceResources();

    // Connects to or listens on the currently set network address
    void ConnectOrListen();

    // Loads the currently saved position of the spinning cube.
    void LoadPosition();

    // Saves the position of the spinning cube.
    void SavePosition();

    // Exports a test anchor via SpatialAnchorExporter.
    winrt::fire_and_forget ExportPosition();

    // Request access for eyes pose data.
    void RequestEyesPoseAccess();

    // Create the perception device handler which is required for qr code tracking.
    winrt::fire_and_forget CreatePerceptionDeviceHandler();

    // Clears event registration state. Used when changing to a new HolographicSpace
    // and when tearing down SampleRemoteMain.
    void UnregisterHolographicEventHandlers();

    // Shuts down the RemoteContext (which will also disconnect, if currently connected)
    void ShutdownRemoteContext();

    // Creates a SwapChain for the host window
    void WindowCreateSwapChain(const winrt::com_ptr<ID3D11Device1>& device);

    // Presents the SwapChain of the host window
    void WindowPresentSwapChain();

    // Updates the title of the host window
    void WindowUpdateTitle();

    // Asynchronously creates resources for new holographic cameras.
    void OnCameraAdded(
        const winrt::Windows::Graphics::Holographic::HolographicSpace& sender,
        const winrt::Windows::Graphics::Holographic::HolographicSpaceCameraAddedEventArgs& args);

    // Synchronously releases resources for holographic cameras that are no longer
    // attached to the system.
    void OnCameraRemoved(
        const winrt::Windows::Graphics::Holographic::HolographicSpace& sender,
        const winrt::Windows::Graphics::Holographic::HolographicSpaceCameraRemovedEventArgs& args);

    // Used to notify the app when the positional tracking state changes.
    void OnLocatabilityChanged(
        const winrt::Windows::Perception::Spatial::SpatialLocator& sender, const winrt::Windows::Foundation::IInspectable& args);

    void OnDisconnected(winrt::Microsoft::Holographic::AppRemoting::ConnectionFailureReason failureReason);

#ifdef ENABLE_CUSTOM_DATA_CHANNEL_SAMPLE
    void OnCustomDataChannelDataReceived();
    void OnCustomDataChannelClosed();
#endif

private:
    bool m_isInitialized = false;

    std::chrono::high_resolution_clock::time_point m_startTime = std::chrono::high_resolution_clock::now();

    // Lock to serialize remote context operations and event handlers
    std::recursive_mutex m_remoteContextAccess;

    // RemoteContext used to connect with a Holographic Remoting player and display rendered frames
    winrt::Microsoft::Holographic::AppRemoting::RemoteContext m_remoteContext = nullptr;

    // Whether a disconnect is currently pending.
    bool m_disconnectPending{false};

    // Represents the holographic space around the user.
    winrt::Windows::Graphics::Holographic::HolographicSpace m_holographicSpace = nullptr;

    // The interaction manager provides an event that informs the app when spatial interactions are detected.
    winrt::Windows::UI::Input::Spatial::SpatialInteractionManager m_interactionManager = nullptr;

    // Cached pointer to device resources.
    std::shared_ptr<DXHelper::DeviceResources> m_deviceResources;

    // SpatialLocator that is attached to the primary camera.
    winrt::Windows::Perception::Spatial::SpatialLocator m_locator = nullptr;

    // A reference frame that is positioned in the world.
    winrt::Windows::Perception::Spatial::SpatialStationaryFrameOfReference m_referenceFrame = nullptr;

    // Renders a colorful holographic cube that's 20 centimeters wide. This sample content
    // is used to demonstrate world-locked rendering.
    std::unique_ptr<SpinningCubeRenderer> m_spinningCubeRenderer;

    // Renders the surface observed in the user's surroundings.
    std::unique_ptr<SpatialSurfaceMeshRenderer> m_spatialSurfaceMeshRenderer;

    // Listens for the Pressed spatial input event.
    std::shared_ptr<SpatialInputHandler> m_spatialInputHandler;
    std::shared_ptr<SpatialInputRenderer> m_spatialInputRenderer;

    // Handles perception root objects and their events/updates
    std::shared_ptr<PerceptionDeviceHandler> m_perceptionDeviceHandler;
    std::unique_ptr<QRCodeRenderer> m_qrCodeRenderer;

    // Event registration tokens.
    winrt::event_token m_cameraAddedToken;
    winrt::event_token m_cameraRemovedToken;
    winrt::event_token m_locatabilityChangedToken;

    // Event registration revokers
    winrt::Microsoft::Holographic::AppRemoting::IRemoteContext::OnConnected_revoker m_onConnectedEventRevoker;
    winrt::Microsoft::Holographic::AppRemoting::IRemoteContext::OnDisconnected_revoker m_onDisconnectedEventRevoker;
    winrt::Microsoft::Holographic::AppRemoting::IRemoteContext::OnSendFrame_revoker m_onSendFrameEventRevoker;
    winrt::Microsoft::Holographic::AppRemoting::IRemoteContext::OnDataChannelCreated_revoker m_onDataChannelCreatedEventRevoker;
    winrt::Microsoft::Holographic::AppRemoting::IRemoteSpeech::OnRecognizedSpeech_revoker m_onRecognizedSpeechRevoker;

    // Host options
    std::wstring m_hostname;
    uint16_t m_port{0};
    uint16_t m_transportPort{0};
    bool m_ephemeralPort = false;
    bool m_showPreview = true;
    bool m_listen{false};

    // Host window related variables
    std::weak_ptr<IWindow> m_window;
    int m_width = INITIAL_WINDOW_WIDTH;
    int m_height = INITIAL_WINDOW_HEIGHT;

    std::chrono::high_resolution_clock::time_point m_windowTitleUpdateTime;
    uint32_t m_framesPerSecond = 0;

    std::recursive_mutex m_deviceLock;
    winrt::com_ptr<IDXGISwapChain1> m_swapChain;
    winrt::com_ptr<ID3D11Texture2D> m_spTexture;

    bool m_canCommitDirect3D11DepthBuffer = false;
    bool m_commitDirect3D11DepthBuffer = true;

    bool m_isStandalone = false;

#ifdef ENABLE_CUSTOM_DATA_CHANNEL_SAMPLE
    std::recursive_mutex m_customDataChannelLock;
    winrt::Microsoft::Holographic::AppRemoting::IDataChannel2 m_customDataChannel = nullptr;
    winrt::Microsoft::Holographic::AppRemoting::IDataChannel2::OnDataReceived_revoker m_customChannelDataReceivedEventRevoker;
    winrt::Microsoft::Holographic::AppRemoting::IDataChannel2::OnClosed_revoker m_customChannelClosedEventRevoker;
    std::chrono::high_resolution_clock::time_point m_customDataChannelSendTime = std::chrono::high_resolution_clock::now();
#endif
};
