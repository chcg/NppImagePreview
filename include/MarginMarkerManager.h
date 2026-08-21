#pragma once

#include <Windows.h>
#include <vector>
#include <set>
#include "ImageParser.h"

class MarginMarkerManager {
public:
    MarginMarkerManager();
    ~MarginMarkerManager();
    
    bool Initialize(HWND hSci);
    void Shutdown();
    
    void UpdateMarkers(const std::vector<ImageRef>& images);
    void ClearMarkers();
    
    bool IsOnMarker(int line);
    bool IsInitialized() const { return m_initialized; }
    
private:
    void SetupMarker();
    
    HWND m_hSci = nullptr;
    int m_markerNumber = 1;
    bool m_initialized = false;
    std::set<int> m_markedLines;
};