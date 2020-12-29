#include "stdafx.h"
#include "Analytics.h"

Analytics::Analytics()
{

}

Analytics::~Analytics()
{

}

void Analytics::imgui()
{
    ImPlot::ShowDemoWindow();
    //ImGui::TestDateChooser();

	ImGui::Begin("Analytics");

    static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f, 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f, 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
    ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));

    // Create a dummy array of contiguous float values to plot
    // Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float
    // and the sizeof() of your structure in the "stride" parameter.
    static float values[90] = {};
    static int values_offset = 0;
    static double refresh_time = 0.0;
    //if (!animate || refresh_time == 0.0)
        refresh_time = ImGui::GetTime();
    while (refresh_time < ImGui::GetTime()) // Create dummy data at fixed 60 Hz rate for the demo
    {
        static float phase = 0.0f;
        values[values_offset] = cosf(phase);
        values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
        phase += 0.10f * values_offset;
        refresh_time += 1.0f / 60.0f;
    }

    // Plots can display overlay texts
    // (in this example, we will display an average value)
    {
        float average = 0.0f;
        for (int n = 0; n < IM_ARRAYSIZE(values); n++)
            average += values[n];
        average /= (float)IM_ARRAYSIZE(values);
        char overlay[32];
        sprintf(overlay, "avg %f", average);
        ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, overlay, -1.0f, 1.0f, ImVec2(0, 80.0f));
    }
    ImGui::PlotHistogram("Histogram", arr, IM_ARRAYSIZE(arr), 0, "Overlay Text", 0.0f, 1.0f, ImVec2(0, 80.0f));

	ImGui::End();
}