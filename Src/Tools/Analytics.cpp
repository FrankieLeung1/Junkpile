#include "stdafx.h"
#include "Analytics.h"

Analytics::Analytics():
m_startTime(),
m_endTime(),
m_currentGraphType(0),
m_singleDate(false)
{
    ImGui::SetDateToday(&m_startTime); setToMidnight(&m_startTime); m_startTime.tm_year -= 1; m_startTime.tm_mday = 0;
    ImGui::SetDateToday(&m_endTime); setToMidnight(&m_endTime);

    m_runs.assign(1000, {});
    for (auto& run : m_runs)
    {
        tm time = m_endTime;
        time.tm_year -= 1;
        time.tm_mon = ((std::rand() * 12) / RAND_MAX); 
        time.tm_mday = (std::rand() * 30) / RAND_MAX; // good enough

        run.m_time = mktime(&time);

        run.m_average = ((std::rand() * 3.0f) / RAND_MAX);
        run.m_items[0] = (Item)((std::rand() * Item::ItemTotal) / RAND_MAX);
        run.m_items[1] = (Item)((std::rand() * Item::ItemTotal) / RAND_MAX);
        run.m_items[2] = (Item)((std::rand() * Item::ItemTotal) / RAND_MAX);
        run.m_armour = (Armour)((std::rand() * Armour::ArmourTotal) / RAND_MAX);
        run.m_snax = (Snax)((std::rand() * Snax::SnaxTotal) / RAND_MAX);
    }

    auto it = std::unique(m_runs.begin(), m_runs.end(), [](const Run& r1, const Run& r2) { return r1.m_time == r2.m_time; });
    m_runs.erase(it, m_runs.end());

    std::sort(m_runs.begin(), m_runs.end(), [](const Run& r1, const Run& r2) { return r2.m_time > r1.m_time; });

    tm ss = { 0 };
    //ImGui::SetDateToday(&ss);
    ss.tm_year = 2020 - 1900;
    ss.tm_mon = 10;
    ss.tm_mday = 15;

    time_t switchStart = mktime(&ss);
    int i = 0, switchIndex = 0;
    float skew = 0.1f;
    for (auto& run : m_runs)
    {
        run.m_count = skew; skew *= 1.004f;
        run.m_dgCount = std::max<float>(0.0f, run.m_count - 0.3f - (std::rand() * 0.1f / RAND_MAX));

        run.m_iOSCount = (float)i / 1000.0f;

        if (run.m_time >= switchStart)
        {
            run.m_switchCount = (switchIndex++ * 1.2f) / 100.0f;
        }
        else
        {
            run.m_switchCount = 0;
        }

        i++;
    }

    updateGraphData();
}

Analytics::~Analytics()
{

}

void Analytics::imgui()
{
    ImPlot::ShowDemoWindow();

    ImGui::Begin("Analytics");

    const float singleWidth = 100.0f;
    float availableWidth = ImGui::GetWindowContentRegionWidth() - singleWidth;
    const float dateSpaceInBetween = 50.0f;
    const float dateWidth = availableWidth * 0.5f - dateSpaceInBetween * 0.5f;
    ImGui::SetNextItemWidth(dateWidth);
    bool dateChanged = ImGui::DateChooser("##start", m_startTime, "%B %d, %Y"); ImGui::SameLine();
    if (!m_singleDate)
    {
        ImGui::SetNextItemWidth(dateSpaceInBetween);
        ImGui::Text(" - "); ImGui::SameLine();
        ImGui::SetNextItemWidth(dateWidth);
        dateChanged = dateChanged || ImGui::DateChooser("##end", m_endTime, "%B %d, %Y");
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(singleWidth);
    dateChanged = dateChanged || ImGui::Checkbox("Single", &m_singleDate);

    if (dateChanged)
        updateGraphData();

    availableWidth += singleWidth;

    ImGui::BeginChild("##Content");

    ImGui::SetNextItemWidth(availableWidth);
    ImGui::Combo("##Type", &m_currentGraphType, "Runs\0Avg. Levels\0Platform\0\0");

    //ImPlot::SetNextPlotLimits(-0.5, 9.5, 0, 110, ImGuiCond_Always);

    char labels[2][128];
    strftime(labels[0], 128, "%b %d, %Y", &m_startTime);
    strftime(labels[1], 128, "%b %d, %Y", &m_endTime);

    {
        const char* l[2] = { (const char*)&labels[0], (const char*)&labels[1] };
        static const double positions[] = { 0, 1 };
        ImPlot::SetNextPlotTicksX(positions, 2, l);
    }

    if (m_currentGraphType == 2)
    {
        if (!m_runsPlotX.empty() && ImPlot::BeginPlot("Platform", NULL, NULL)) 
        {
            ImPlot::SetNextLineStyle(ImVec4(0.7f, 0.7f, 0.7f, 0.5f), 1.0f);
            ImPlot::PlotLine("iOS", &m_runsPlotX[0], &m_iosPlot[0], (int)m_iosPlot.size());
            ImPlot::SetNextLineStyle(ImVec4(1, 0, 0, 0.5f), 1.0f);
            ImPlot::PlotLine("Switch", &m_runsPlotX[0], &m_switchPlot[0], (int)m_switchPlot.size());

            ImPlot::EndPlot();
        }
    }
    else
    {
        if (!m_runsPlotX.empty() && ImPlot::BeginPlot("##Scatter Plot", NULL, NULL))
        {
            if (m_currentGraphType == 0)
            {
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                ImPlot::PlotShaded("Run Count", &m_runsPlotX[0], &m_runsPlotY[0], (int)m_runsPlotY.size());
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 6, ImVec4(0, 1, 0, 0.5f), IMPLOT_AUTO, ImVec4(0, 1, 0, 1));
                ImPlot::PlotShaded("DG Count", &m_runsPlotX[0], &m_dgPlotY[0], (int)m_dgPlotY.size());
                ImPlot::PopStyleVar();

                ImPlot::PlotLine("Run Count", &m_runsPlotX[0], &m_runsPlotY[0], (int)m_runsPlotY.size());
                ImPlot::PlotLine("DG Count", &m_runsPlotX[0], &m_dgPlotY[0], (int)m_dgPlotY.size());
            }
            else
            {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 1, ImVec4(0, 1, 0, 0.5f), IMPLOT_AUTO, ImVec4(0, 1, 0, 1));
                ImPlot::PlotScatter("Average", &m_runsPlotX[0], &m_averagePlotY[0], (int)m_averagePlotY.size());
            }
            ImPlot::EndPlot();
        }
    }
    ImGui::Text("%d data points", m_runsPlotX.size());

    bool normalize = true;
    if (!m_itemsPlot.empty() && ImPlot::BeginPlot("Items Used", NULL, NULL, ImVec2(availableWidth, availableWidth), ImPlotFlags_Equal | ImPlotFlags_NoMousePos, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations)) {
        ImPlot::PlotPieChart(itemStrings, &m_itemsPlot[0], (int)m_itemsPlot.size(), ((availableWidth / 3) + (availableWidth / 3)) / availableWidth, 0.5, 0.3, normalize, "%.0f");
        ImPlot::EndPlot();
    }

    if (!m_armourPlot.empty() && ImPlot::BeginPlot("Armour Used", NULL, NULL, ImVec2(availableWidth, availableWidth), ImPlotFlags_Equal | ImPlotFlags_NoMousePos, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations)) {
        ImPlot::PlotPieChart(armourStrings, &m_armourPlot[0], (int)m_armourPlot.size(), ((availableWidth / 3) + (availableWidth / 3)) / availableWidth, 0.5, 0.3, normalize, "%.0f");
        ImPlot::EndPlot();
    }

    {
        static const double positions[] = { 0.7f, 3.2f, 5.7f, 8.2f };
        ImPlot::SetNextPlotTicksX(positions, 4, &snaxString[0]);
    }
    ImPlot::SetNextPlotLimits(-0.5, 9.5, 0, 300, ImGuiCond_Once);
    if (ImPlot::BeginPlot("Snax", "Type", "Amount", ImVec2(-1, 0), 0, 0, 0))
    {
        auto getter = [](void* data, int idx) { return ImPlotPoint(idx * 2.5f + 0.7f, ((float*)data)[idx]); };
        ImPlot::PlotBarsG("Snax", getter, &m_snax[0], (int)m_snax.size(), 1.2f);
        ImPlot::EndPlot();
    }

    // platform (iOS/tvOS/macOS/Switch)

    ImGui::EndChild();
	ImGui::End();
}

void Analytics::updateGraphData()
{
    m_runsPlotX.clear();
    m_runsPlotY.clear();
    m_iosPlot.clear();
    m_switchPlot.clear();
    m_dgPlotY.clear();
    m_itemsPlot.assign(Item::ItemTotal, 0.0f);
    m_armourPlot.assign(Armour::ArmourTotal, 0.0f);
    m_snax.assign(Snax::SnaxTotal, 0.0f);

    setToMidnight(&m_startTime);
    setToMidnight(&m_endTime);
    
    time_t startTime = mktime(&m_startTime), endTime = mktime(&m_endTime);
    time_t currentTime = startTime;
    if (m_singleDate)
    {
        endTime = startTime;
        endTime += 86400; // seconds in a day
    }
    for (auto& run : m_runs)
    {
        bool isBefore = run.m_time < startTime, isAfter = run.m_time >= endTime;
        if (isBefore || isAfter)
            continue;

        float x = (float)(currentTime - startTime) / (float)(endTime - startTime);
        m_runsPlotX.push_back(x);
        m_runsPlotY.push_back(run.m_count);
        m_iosPlot.push_back(run.m_iOSCount);
        m_switchPlot.push_back(run.m_switchCount);
        m_dgPlotY.push_back(run.m_dgCount);
        m_averagePlotY.push_back(run.m_average);
        m_itemsPlot[run.m_items[0]] += 1.0f;
        m_itemsPlot[run.m_items[1]] += 1.0f;
        m_itemsPlot[run.m_items[2]] += 1.0f;
        m_armourPlot[run.m_armour] += 1.0f;
        m_snax[run.m_snax] += 1.0f;
        currentTime = run.m_time;
    }
}

void Analytics::setToMidnight(tm* t) const
{
    t->tm_sec = 0;
    t->tm_min = 0;
    t->tm_hour = 0;
}