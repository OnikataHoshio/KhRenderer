#include "KH_Window.h"
#include "KH_Camera.h"

#include "KH_Editor.h"

KH_Camera* KH_Window::Camera = nullptr;

bool KH_Window::bCameraLocked = true;

float KH_Window::LastMouseX = 0.0f;
float KH_Window::LastMouseY = 0.0f;
bool KH_Window::bFirstMouse = true;

KH_Window::KH_Window(uint32_t Width, uint32_t Height, std::string Title)
	:Width(Width), Height(Height), Title(Title)
{
	Initialize();
}

KH_Window::~KH_Window()
{
	DeInitialize();
}

void KH_Window::AddInputCallback(const InputCallback& callback)
{
    InputCallbacks.push_back(callback);
}

void KH_Window::AddResizeCallback(const ResizeCallback& callback)
{
    ResizeCallbacks.push_back(callback);
}

void KH_Window::BeginRender()
{
    glfwPollEvents();
    ProcessData();
    ProcessInput(Window);
}

void KH_Window::EndRender()
{
    glfwSwapBuffers(Window);
}

void KH_Window::Initialize()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    const char* glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    Window = glfwCreateWindow(
        Width, Height,
        Title.c_str(),
        nullptr, nullptr
    );

    if (Window == nullptr)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
    }

    glfwMakeContextCurrent(Window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    glViewport(0, 0, KH_Editor::CanvasWidth, KH_Editor::CanvasHeight);
    glfwSetWindowUserPointer(Window, this);
    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(Window, MouseMovementCallback);
    glfwSetScrollCallback(Window, MouseScrollCallback);
    glfwSetMouseButtonCallback(Window, MouseButtonCallback);

    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSwapInterval(1);

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void KH_Window::DeInitialize()
{
	ImGui::DestroyPlatformWindows();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(Window);
    glfwTerminate();
}

void KH_Window::ProcessData()
{
    float CurrentFrame = static_cast<float>(glfwGetTime());
    DeltaTime = CurrentFrame - LastFrame;
    LastFrame = CurrentFrame;
}

void KH_Window::ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);


    if (!bCameraLocked)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Forward, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Left, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Backward, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Right, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Up, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Down, DeltaTime);
    }


    auto* self = static_cast<KH_Window*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    for (const auto& callback : self->InputCallbacks)
    {
        callback(window);
    }
}


void KH_Window::SetCamera(KH_Camera* Camera)
{
    this->Camera = Camera;
}

void KH_Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    //glViewport(0, 0, width, height);

    auto* self = static_cast<KH_Window*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    for (const auto& callback : self->ResizeCallbacks)
    {
        callback(window, width, height);
    }
}

void KH_Window::MouseMovementCallback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!Camera || bCameraLocked) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (bFirstMouse)
    {
        LastMouseX = xpos;
        LastMouseY = ypos;
        bFirstMouse = false;
    }

    float OffsetX = xpos - LastMouseX;
    float OffsetY = LastMouseY - ypos; // 注意这里反过来

    LastMouseX = xpos;
    LastMouseY = ypos;

    Camera->ProcessMouseMovement(OffsetX, OffsetY);
}

void KH_Window::MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Camera->ProcessMouseScroll(yoffset);
}

void KH_Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        bCameraLocked = false;
        bFirstMouse = true;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        bCameraLocked = true;
        bFirstMouse = true;
    }
}

//TODO::ADD Funcction to CallbackVector
void KH_Window::WindowCloseCallback(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void KH_Window::CameraMovementCallback(GLFWwindow* window)
{
    if (!bCameraLocked)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Forward, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Left, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Backward, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Right, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Up, DeltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            Camera->ProcessKeyboard(CameraMovement::Down, DeltaTime);
    }
}

void KH_Window::CameraLockedCallback(GLFWwindow* window)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        bCameraLocked = false;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        bCameraLocked = true;
        bFirstMouse = true;
    }
}
