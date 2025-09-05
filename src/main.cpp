#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/CustomGeometry.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RenderPipeline/RenderPipeline.h>
#include <Urho3D/RenderPipeline/ShaderConsts.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/SystemUI/DebugHud.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

using namespace Urho3D;

class MyApp : public Application
{
    URHO3D_OBJECT(MyApp, Application)

public:
    MyApp(Context* context) : Application(context), yaw_(0.0f), pitch_(0.0f), drawDebug_(false) {}

    virtual void Setup() override
    {
        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_WINDOW_WIDTH] = 1280;
        engineParameters_[EP_WINDOW_HEIGHT] = 720;
        engineParameters_[EP_WINDOW_RESIZABLE] = true;
        engineParameters_[EP_BORDERLESS] = false;
    }

    virtual void Start() override
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();

        // Create scene
        scene_ = new Scene(context_);
        octree_ = scene_->CreateComponent<Octree>();
        DebugRenderer * const debugRenderer = scene_->CreateComponent<DebugRenderer>();
        zone_ = scene_->CreateComponent<Zone>();
        zone_->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
        zone_->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
        zone_->SetFogColor(Color(0.5f, 0.5f, 0.7f));
        zone_->SetFogStart(100.0f);
        zone_->SetFogEnd(300.0f);

        {
            RenderPipeline * const renderPipeline = scene_->CreateComponent<RenderPipeline>();
            RenderPipelineSettings settings = renderPipeline->GetSettings();
            settings.renderBufferManager_.readableDepth_ = true;
            // settings.sceneProcessor_.directionalShadowSize_ = 2048;
            // settings.sceneProcessor_.spotShadowSize_ = 2048;
            settings.sceneProcessor_.pointShadowSize_ = 1024;
            // settings.shadowMapAllocator_.shadowAtlasPageSize_ = 8192;
            // TODO how to set the shadow map quality to 16-bit vs 32-bit?
            renderPipeline->SetSettings(settings);
            renderPipeline->SetRenderPassEnabled(eastl::string("Postprocess: SSAO"), true);
        }

        // Create procedural plane model for floor
        CreateFloor();

        // Create cubes
        CreateCube(Vector3(-2.0f, 0.5f, 0.0f), Color(1.0f, 0.0f, 0.0f));
        CreateCube(Vector3(2.0f, 0.5f, 0.0f), Color(0.0f, 1.0f, 0.0f));
        CreateCube(Vector3(0.0f, 0.5f, 2.0f), Color(0.0f, 0.0f, 1.0f));
        // Stacked cubes
        CreateCube(Vector3(0.0f, 0.5f, -2.0f), Color(1.0f, 1.0f, 0.0f));
        CreateCube(Vector3(0.0f, 1.5f, -2.0f), Color(0.0f, 1.0f, 1.0f));

        // Point light
        Node* lightNode = scene_->CreateChild("PointLight");
        lightNode->SetPosition(Vector3(5.0f, 3.0f, 5.0f));
        Light* light = lightNode->CreateComponent<Light>();
        light->SetLightType(LIGHT_POINT);
        light->SetRange(50.0f);
        light->SetBrightness(1.5f);
        light->SetColor(Color(1.0f, 1.0f, 1.0f));
        light->SetCastShadows(true);
        // light->SetShadowResolution(2048);
        light->SetShadowBias(BiasParameters(0.0001f, -0.1f, 0.001));
        light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

        // Camera
        cameraNode_ = scene_->CreateChild("Camera");
        cameraNode_->SetPosition(Vector3(0.0f, 5.0f, -20.0f));
        Camera* camera = cameraNode_->CreateComponent<Camera>();
        camera->SetFarClip(300.0f);

        // Viewport
        Renderer* renderer = GetSubsystem<Renderer>();
        SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));
        renderer->SetViewport(0, viewport);

        // Debug HUD for FPS
        XMLFile* xml = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
        debugHud_ = engine_->CreateDebugHud();
        // debugHud_->SetDefaultStyle(xml); // ADDED
        // Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
        // Note: Assuming the engine's default font resource is available; this is minimal as it's bundled with Urho3D/rbfx.

        // Set mouse mode for FPS control
        Input* input = GetSubsystem<Input>();
        input->SetMouseVisible(false);
        input->SetMouseMode(MM_RELATIVE);

        // Subscribe to events
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MyApp, HandleKeyDown));
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MyApp, HandleUpdate));
        SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(MyApp, HandleMouseMove));
        SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(MyApp, HandlePostRenderUpdate));
    }

    virtual void Stop() override {}

    void HandleKeyDown(StringHash eventType, VariantMap& eventData)
    {
        int key = eventData[KeyDown::P_KEY].GetInt();
        if (key == KEY_ESCAPE) {
            engine_->Exit();
        }
    }

    void HandleUpdate(StringHash eventType, VariantMap& eventData)
    {
        float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
        Input* input = GetSubsystem<Input>();

        // Movement
        float moveSpeed = 10.0f;
        if (input->GetKeyDown(KEY_W)) cameraNode_->Translate(Vector3::FORWARD * moveSpeed * timeStep);
        if (input->GetKeyDown(KEY_S)) cameraNode_->Translate(Vector3::BACK * moveSpeed * timeStep);
        if (input->GetKeyDown(KEY_A)) cameraNode_->Translate(Vector3::LEFT * moveSpeed * timeStep);
        if (input->GetKeyDown(KEY_D)) cameraNode_->Translate(Vector3::RIGHT * moveSpeed * timeStep);

        if (input->GetKeyPress(KEY_SPACE))
            drawDebug_ = !drawDebug_;

        // Update debug HUD (shows FPS)
        debugHud_->SetMode(DEBUGHUD_SHOW_ALL);
    }

    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
    {
        // If draw debug mode is enabled, draw viewport debug geometry. Disable depth test so that we can see the effect of occlusion
        if (drawDebug_)
            GetSubsystem<Renderer>()->DrawDebugGeometry(false);
    }

    void HandleMouseMove(StringHash eventType, VariantMap& eventData)
    {
        Input* input = GetSubsystem<Input>();
        if (input->GetMouseMode() != MM_RELATIVE) return;

        int dx = eventData[MouseMove::P_DX].GetInt();
        int dy = eventData[MouseMove::P_DY].GetInt();

        const float mouseSensitivity = 0.2f;
        yaw_ += dx * mouseSensitivity;
        pitch_ += dy * mouseSensitivity;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

private:
    void CreateFloor()
    {
        // Create floor plane
        Node* floorNode = scene_->CreateChild("Floor");
        floorNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        CustomGeometry* floorGeom = floorNode->CreateComponent<CustomGeometry>();
        floorGeom->BeginGeometry(0, TRIANGLE_LIST);
        // Define square plane (two triangles)
        floorGeom->DefineVertex(Vector3(-10.0f, 0.0f, 10.0f)); floorGeom->DefineNormal(Vector3::UP); floorGeom->DefineTexCoord(Vector2(0.0f, 1.0f));
        floorGeom->DefineVertex(Vector3(10.0f, 0.0f, 10.0f)); floorGeom->DefineNormal(Vector3::UP); floorGeom->DefineTexCoord(Vector2(1.0f, 1.0f));
        floorGeom->DefineVertex(Vector3(-10.0f, 0.0f, -10.0f)); floorGeom->DefineNormal(Vector3::UP); floorGeom->DefineTexCoord(Vector2(0.0f, 0.0f));
        floorGeom->DefineVertex(Vector3(10.0f, 0.0f, 10.0f)); floorGeom->DefineNormal(Vector3::UP); floorGeom->DefineTexCoord(Vector2(1.0f, 1.0f));
        floorGeom->DefineVertex(Vector3(10.0f, 0.0f, -10.0f)); floorGeom->DefineNormal(Vector3::UP); floorGeom->DefineTexCoord(Vector2(1.0f, 0.0f));
        floorGeom->DefineVertex(Vector3(-10.0f, 0.0f, -10.0f)); floorGeom->DefineNormal(Vector3::UP); floorGeom->DefineTexCoord(Vector2(0.0f, 0.0f));
        floorGeom->Commit();
        floorGeom->SetMaterial(CreateMaterial(Color(0.8f, 0.8f, 0.8f)));
        floorGeom->SetCastShadows(false);  // Floor doesn't cast shadows
    }

    void CreateCube(const Vector3& pos, const Color& color)
    {
        Node* cubeNode = scene_->CreateChild("Cube");
        cubeNode->SetPosition(pos);
        cubeNode->SetScale(Vector3(1.0f, 1.0f, 1.0f));
        CustomGeometry* cubeGeom = cubeNode->CreateComponent<CustomGeometry>();
        cubeGeom->BeginGeometry(0, TRIANGLE_LIST);

        // Define cube vertices (positions, normals; no texcoords needed)
        // Rear face (Z = -0.5, outward normal BACK)
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::BACK);
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::BACK);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::BACK);
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::BACK);
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::BACK);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::BACK);
        // Front face (Z = 0.5, outward normal FORWARD)
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::FORWARD);
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::FORWARD);
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::FORWARD);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::FORWARD);
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::FORWARD);
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::FORWARD);
        // Left face (X = -0.5, outward normal LEFT)
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::LEFT);
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::LEFT);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::LEFT);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::LEFT);
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::LEFT);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::LEFT);
        // Right face (X = 0.5, outward normal RIGHT)
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::RIGHT);
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::RIGHT);
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::RIGHT);
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::RIGHT);
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::RIGHT);
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::RIGHT);
        // Bottom face (Y = -0.5, outward normal DOWN)
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::DOWN);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::DOWN);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::DOWN);
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::DOWN);
        cubeGeom->DefineVertex(Vector3(-0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::DOWN);
        cubeGeom->DefineVertex(Vector3(0.5f, -0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::DOWN);
        // Top face (Y = 0.5, outward normal UP)
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::UP);
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::UP);
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::UP);
        cubeGeom->DefineVertex(Vector3(-0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::UP);
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, 0.5f)); cubeGeom->DefineNormal(Vector3::UP);
        cubeGeom->DefineVertex(Vector3(0.5f, 0.5f, -0.5f)); cubeGeom->DefineNormal(Vector3::UP);

        cubeGeom->Commit();
        cubeGeom->SetMaterial(CreateMaterial(color));
        cubeGeom->SetCastShadows(true);
    }

    SharedPtr<Material> CreateMaterial(const Color& color)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        SharedPtr<Material> mat(new Material(context_));
        mat->SetTechnique(0, cache->GetResource<Technique>("Techniques/NoTextureAO.xml"));
        mat->SetShaderParameter(ShaderConsts::Material_MatDiffColor, color);
        mat->SetShadowCullMode(CULL_CW);
        return mat;
    }

    SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
    SharedPtr<DebugHud> debugHud_;
    SharedPtr<Octree> octree_;
    SharedPtr<Zone> zone_;
    float yaw_;
    float pitch_;
    bool drawDebug_;
};

URHO3D_DEFINE_APPLICATION_MAIN(MyApp);
