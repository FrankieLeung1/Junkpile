pressed = False

def imgui(e):
    global pressed

    ImGui.Begin("Test")
    if(ImGui.Button("button")):
        pressed = not pressed

    ImGui.Separator()
    
    if(pressed):
        ImGui.Text("BUTTON PRESSED!")

    ImGui.End()

eventManager.addListener_ImGuiRender(imgui, 0)