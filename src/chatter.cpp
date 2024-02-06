#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <SDL.h>
#include <glad/glad.h>
// #include <SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <stdio.h>

#include <vector>
#include <thread>

#include "chatter.h"

#define WIDTH 1028
#define HEIGHT 720

#define BUFLEN 512
#define PORT "27015"

enum ApplicationState {
  Application_Login,
  Application_Connecting,
  Application_Chat
};

ApplicationState application_state = Application_Login;

std::vector<Message> messages;

SOCKET sc_socket;
bool listening = true;

void start_client(char *ip_address, char *port) {
  int err = 0;
  char recvbuf[512];
  int recvbuf_len = 512;

  struct addrinfo *result = NULL, *ptr = NULL, hints{};

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  err = getaddrinfo(ip_address, port, &hints, &result);
  if (err) {
    printf("getaddrinfo failed, error: %d\n", err);
  }

  SOCKET connect_socket = INVALID_SOCKET;
  for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
    connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (connect_socket == INVALID_SOCKET) {
      printf("socket failed with error: %d\n", WSAGetLastError());
    }
    err = connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (err == SOCKET_ERROR) {
      closesocket(connect_socket);
      connect_socket = INVALID_SOCKET;
      continue;
    }
    break;
  }

  freeaddrinfo(result);

  sc_socket = connect_socket;
  if (connect_socket == INVALID_SOCKET) {
    printf("Unable to connect to server\n");
  }

  application_state = Application_Chat;
  // printf("receiving messages...");

  int bytes_ret = 0;
  for (;;) {
    bytes_ret = recv(connect_socket, recvbuf, recvbuf_len, 0);
    if (bytes_ret > 0) {
      // printf("Bytes received: %d\n", bytes_ret);
      char *msg = (char *)malloc(bytes_ret + 1);
      strcpy(msg, recvbuf);
      msg[bytes_ret] = '\0';
      Message message{};
      message.text = msg;
      messages.push_back(message);
    } else if (bytes_ret == 0) {
      printf("Connection closed.\n");
    } else {
      printf("recv failed with error: %d\n", WSAGetLastError());
    }
  }

  // closesocket(connect_socket);
  // WSACleanup();
}

void start_server(char *port) {
  int err = 0;
  struct addrinfo *result = NULL;
  struct addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  err = getaddrinfo(NULL, port, &hints, &result);
  if (err) {
    printf("getaddrinfo failed, error:%d\n", WSAGetLastError());
    return;
  }

  // socket to listen for client connections.
  SOCKET listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (listen_socket == INVALID_SOCKET) {
    printf("socket failed, error:%d\n", WSAGetLastError());
    freeaddrinfo(result);
    return;
  }

  err = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
  if (err == SOCKET_ERROR) {
    printf("bind failed, error:%d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(listen_socket);
    return;
  }

  freeaddrinfo(result);

  err = listen(listen_socket, SOMAXCONN);
  if (err == SOCKET_ERROR) {
    printf("listen failed, error:%d\n", WSAGetLastError());
    closesocket(listen_socket);
    return;
  }

  SOCKET client_socket = accept(listen_socket, NULL, NULL);
  sc_socket = client_socket;
  if (client_socket == INVALID_SOCKET) {
    printf("accept failed, error:%d\n", WSAGetLastError());
    closesocket(listen_socket);
    return;
  }
  closesocket(listen_socket);

  // NOTE: go into chatbox mode, done connecting
  application_state = Application_Chat;

  // printf("receiving messages...\n");
  
#define BUFLEN 512
  
  char recvbuf[BUFLEN] = {};
  int recvbuf_len = BUFLEN;
  int bytes_ret = 0;
  for (;;) {
    bytes_ret = recv(client_socket, recvbuf, recvbuf_len, 0);
    if (bytes_ret > 0) {
      printf("message received:\n%s\n", recvbuf);
      char *msg = (char *)malloc(bytes_ret + 1);
      memcpy(msg, recvbuf, bytes_ret);
      msg[bytes_ret] = '\0';
      
      Message message = {};
      message.text = msg;
      messages.push_back(message);
    } else if (bytes_ret == 0) {
      // printf("connection closing...\n");
    } else {
      printf("recv failed, error:%d\n", WSAGetLastError());
    }
  }
}

void send_message(char *msg) {
  int bytes_sent = send(sc_socket, msg, (int)strlen(msg), 0);
  if (bytes_sent == SOCKET_ERROR) {
    printf("send failed with err:%d\n", WSAGetLastError());
  }

  Message message{};
  message.text = msg;
  messages.push_back(message);
}

int main(int argc, char **argv) {
  // INITIALIZE SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error:%s\n", SDL_GetError());
    return -1;
  }

  int context_flags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#ifdef _DEBUG
  context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  SDL_Window *window =
      SDL_CreateWindow("chatter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  if (window == nullptr) {
    printf("Window could not be created! SDL_Error:%s\n", SDL_GetError());
    return -1;
  }
  SDL_SetWindowResizable(window, SDL_TRUE);

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == NULL) {
    printf("Failed to create OpenGL context!\n");
    return -1;
  }
  if (SDL_GL_MakeCurrent(window, gl_context) < 0) {
    printf("failed to set opengl context for rendering, error: %s\n", SDL_GetError());
    return -1;
  }
  if (!gladLoadGLLoader((GLADloadproc)(SDL_GL_GetProcAddress))) {
    printf("Failed to initialize GLAD!\n");
    return -1;
  }
  if (SDL_GL_SetSwapInterval(1) < 0) {
    printf("Failed to set swap interval, error:%s\n", SDL_GetError());
  }
  
  // INITIALIZE IMGUI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 130");

  ImGuiStyle *style = &ImGui::GetStyle();
  style->WindowPadding = ImVec2(15, 15);
  style->WindowRounding = 5.0f;
  style->FramePadding = ImVec2(5, 5);
  style->FrameRounding = 4.0f;
  style->ItemSpacing = ImVec2(12, 8);
  style->ItemInnerSpacing = ImVec2(8, 6);
  style->IndentSpacing = 25.0f;
  style->ScrollbarSize = 15.0f;
  style->ScrollbarRounding = 9.0f;
  style->GrabMinSize = 5.0f;
  style->GrabRounding = 3.0f;
 
  style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
  style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
  style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
  style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
  style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
  style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
  style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
  style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
  style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
  style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
  style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
  style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
  style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);

  // INITIALIZE WSA
  WSADATA wsa_data{};

  int err = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (err) {
    printf("Failed to startup WSA: %d\n", err);
    WSACleanup();
    return 1;
  }

  char ip_address[64]{};
  char port[64]{};
  bool is_server = false;

  char text[512] = {};
  
  bool window_should_close = false;
  while (!window_should_close) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type) {
      case SDL_QUIT:
        window_should_close = true;
        break;
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    
    // Make next window fill up the whole window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    switch (application_state) {
      case Application_Login: {
        ImGui::Begin("Connect", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Checkbox("Server", &is_server);
    
        ImGui::InputText("IP Address", ip_address, 64, ImGuiInputTextFlags_CharsDecimal);
        ImGui::InputText("Port", port, 64, ImGuiInputTextFlags_CharsDecimal);
    
        if (ImGui::Button("Connect")) {
          application_state = Application_Connecting;
          
          if (is_server) {
            std::thread thread(start_server, port);
            thread.detach();
          } else {
            std::thread thread(start_client, ip_address, port);
            thread.detach();
          }
        }
    
        if (ImGui::Button("Exit")) {
          window_should_close = true;
        }

        ImGui::End();        
        break;
      }
      case Application_Connecting: {
        ImGui::Begin("Connecting", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Text("Setting up a connection...");
        if (ImGui::Button("Exit")) {
          window_should_close = true;
        }
        ImGui::End();
        break;
      }
      case Application_Chat: {
        ImGui::Begin("Chat", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

        int TEXTBOX_HEIGHT = (int)ImGui::GetTextLineHeight() * 4;
        ImGui::BeginChild("ChildR", ImVec2(0, ImGui::GetContentRegionAvail().y - TEXTBOX_HEIGHT - 27), true, 0);

        // TODO: Format chat history
        // ImGui::Dummy(ImVec2(0, ImGui::GetContentRegionAvail().y));
        ImGui::Dummy(ImVec2(0, 0));
    
        // NOTE: Show the chat messages
        for (Message &message : messages) {
          ImGui::TextWrapped(message.text);
          ImGui::Spacing();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        if (ImGui::InputTextMultiline("text", text, sizeof(text), ImVec2(-FLT_MIN, (float)TEXTBOX_HEIGHT), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CtrlEnterForNewLine) || ImGui::Button("Send")) {
          char *str = (char *)malloc(strlen(text) + 1);
          strcpy(str, text);
          str[strlen(text)] = '\0';
          send_message(str);
          memset(text, 0, sizeof(text));
        }
    
        ImGui::End();
        break;
      }
    }
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  return 0;
}
