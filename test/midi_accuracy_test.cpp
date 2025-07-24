#include <map>
#include <vector>
#include <iostream>
#include "components/midi/MidiMessageRouter.h"

class MidiProcessingAccuracyTest {
private:
    struct EventCounter {
        int note_events = 0;
        int cc_events = 0;
        int clock_events = 0;
        int other_events = 0;
    };
    
    EventCounter sent_events_;
    EventCounter received_events_;
    
public:
    class TestNoteObserver : public TypedObserver<NoteEvent> {
    private:
        MidiProcessingAccuracyTest* test_;
    public:
        TestNoteObserver(MidiProcessingAccuracyTest* test) : test_(test) {}
        void onEvent(const NoteEvent& event) override {
            test_->received_events_.note_events++;
        }
    };
    
    class TestCCObserver : public TypedObserver<CCEvent> {
    private:
        MidiProcessingAccuracyTest* test_;
    public:
        TestCCObserver(MidiProcessingAccuracyTest* test) : test_(test) {}
        void onEvent(const CCEvent& event) override {
            test_->received_events_.cc_events++;
        }
    };
    
    class TestClockObserver : public TypedObserver<ClockEvent> {
    private:
        MidiProcessingAccuracyTest* test_;
    public:
        TestClockObserver(MidiProcessingAccuracyTest* test) : test_(test) {}
        void onEvent(const ClockEvent& event) override {
            test_->received_events_.clock_events++;
        }
    };
    
    void runAccuracyTest() {
        std::cout << "Testing MIDI message processing accuracy..." << std::endl;
        
        MidiMessageRouter router;
        
        // Set up observers
        TestNoteObserver note_observer(this);
        TestCCObserver cc_observer(this);
        TestClockObserver clock_observer(this);
        
        router.getNoteSubject().addObserver(&note_observer);
        router.getCCSubject().addObserver(&cc_observer);
        router.getClockSubject().addObserver(&clock_observer);
        
        // Reset counters
        sent_events_ = {};
        received_events_ = {};
        
        // Send test messages
        sendTestMessages(router);
        
        // Process all queued events
        router.processAllEvents();
        
        // Analyze results
        analyzeAccuracyResults();
    }
    
private:
    void sendTestMessages(MidiMessageRouter& router) {
        // Send various MIDI message types
        
        // Note messages
        for (int i = 0; i < 100; ++i) {
            router.processMidiMessage(MidiMessage::noteOn(60 + (i % 12), 100));
            sent_events_.note_events++;
            
            router.processMidiMessage(MidiMessage::noteOff(60 + (i % 12), 0));
            sent_events_.note_events++;
        }
        
        // CC messages
        for (int i = 0; i < 50; ++i) {
            router.processMidiMessage(MidiMessage::controlChange(1, i % 128, i % 128));
            sent_events_.cc_events++;
        }
        
        // Clock messages
        for (int i = 0; i < 200; ++i) {
            router.processMidiMessage(MidiMessage::clock());
            sent_events_.clock_events++;
        }
        
        // Other messages (should be filtered out or handled separately)
        router.processMidiMessage(MidiMessage::programChange(1, 42));
        sent_events_.other_events++;
        
        std::cout << "Sent: " << sent_events_.note_events << " note, " 
                  << sent_events_.cc_events << " CC, " 
                  << sent_events_.clock_events << " clock events" << std::endl;
    }
    
    void analyzeAccuracyResults() {
        std::cout << "\n=== MIDI Processing Accuracy Results ===" << std::endl;
        
        bool passed = true;
        
        // Check note events
        if (received_events_.note_events != sent_events_.note_events) {
            std::cout << "❌ Note events: sent " << sent_events_.note_events 
                      << ", received " << received_events_.note_events << std::endl;
            passed = false;
        } else {
            std::cout << "✅ Note events: " << received_events_.note_events << std::endl;
        }
        
        // Check CC events
        if (received_events_.cc_events != sent_events_.cc_events) {
            std::cout << "❌ CC events: sent " << sent_events_.cc_events 
                      << ", received " << received_events_.cc_events << std::endl;
            passed = false;
        } else {
            std::cout << "✅ CC events: " << received_events_.cc_events << std::endl;
        }
        
        // Check clock events
        if (received_events_.clock_events != sent_events_.clock_events) {
            std::cout << "❌ Clock events: sent " << sent_events_.clock_events 
                      << ", received " << received_events_.clock_events << std::endl;
            passed = false;
        } else {
            std::cout << "✅ Clock events: " << received_events_.clock_events << std::endl;
        }
        
        if (passed) {
            std::cout << "✅ PASS: All MIDI messages processed correctly" << std::endl;
        } else {
            std::cout << "❌ FAIL: Message processing errors detected" << std::endl;
        }
    }
};
