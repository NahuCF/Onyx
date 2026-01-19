#pragma once

#include <queue>
#include <vector>
#include <cstdint>

namespace MMO {

// ============================================================
// PHASE HELPERS
// ============================================================

// Helper macro for phase bitmask
#define PHASE(n) (1u << (n))
#define PHASE_ALL 0xFFFFFFFF

// ============================================================
// EVENT MAP - Timer-based event scheduling for AI with phases
// ============================================================

class EventMap {
public:
    void Update(float dt) {
        m_CurrentTime += dt;
    }

    // Schedule event for specific phases (default = all phases)
    void ScheduleEvent(uint32_t eventId, float delay, uint32_t phaseMask = PHASE_ALL) {
        m_Events.push({ eventId, m_CurrentTime + delay, phaseMask });
    }

    // Cancel all instances of an event
    void CancelEvent(uint32_t eventId) {
        std::vector<Event> remaining;
        while (!m_Events.empty()) {
            Event e = m_Events.top();
            m_Events.pop();
            if (e.id != eventId) {
                remaining.push_back(e);
            }
        }
        for (const auto& e : remaining) {
            m_Events.push(e);
        }
    }

    // Returns next ready event ID + 1, or 0 if none ready
    // Only returns events that match current phase
    uint32_t ExecuteEvent() {
        // Skip events that don't match current phase
        while (!m_Events.empty() && m_Events.top().time <= m_CurrentTime) {
            Event e = m_Events.top();
            m_Events.pop();

            // Check phase match
            if (e.phaseMask & m_PhaseMask) {
                return e.id + 1;  // Return ID + 1 so 0 means "no event"
            }
            // Wrong phase - discard (don't reschedule, let caller handle)
        }

        if (m_Events.empty()) return 0;
        if (m_Events.top().time > m_CurrentTime) return 0;

        return 0;
    }

    bool HasPendingEvents() const {
        return !m_Events.empty();
    }

    void Reset() {
        while (!m_Events.empty()) m_Events.pop();
        m_CurrentTime = 0.0f;
        m_PhaseMask = PHASE(1);  // Default to phase 1
    }

    float GetTime() const { return m_CurrentTime; }

    // Phase management
    void SetPhase(uint32_t phase) { m_PhaseMask = PHASE(phase); }
    void AddPhase(uint32_t phase) { m_PhaseMask |= PHASE(phase); }
    void RemovePhase(uint32_t phase) { m_PhaseMask &= ~PHASE(phase); }
    uint32_t GetPhaseMask() const { return m_PhaseMask; }
    bool IsInPhase(uint32_t phase) const { return m_PhaseMask & PHASE(phase); }

private:
    struct Event {
        uint32_t id;
        float time;
        uint32_t phaseMask;
    };

    struct EventCompare {
        bool operator()(const Event& a, const Event& b) const {
            return a.time > b.time;  // Min-heap by time
        }
    };

    std::priority_queue<Event, std::vector<Event>, EventCompare> m_Events;
    float m_CurrentTime = 0.0f;
    uint32_t m_PhaseMask = PHASE(1);  // Active phase bitmask (default phase 1)
};

} // namespace MMO
