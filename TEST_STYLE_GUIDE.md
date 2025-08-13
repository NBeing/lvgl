# Test Comment Style Guide

## 📝 Professional Test Documentation Standards

### Story-Driven Test Structure

Follow the excellent example from `test/RTSafeUIControlIntegrationTests.cpp`:

```cpp
/**
 * TEST: Clear descriptive name of what is being tested
 * 
 * SCENARIO: Real-world situation that this test represents:
 *           - Specific user actions or system conditions
 *           - Hardware/software environment details
 *           - Timing or performance constraints
 *           - Multiple actors (user, MIDI, automation, etc.)
 * 
 * VALIDATES:
 * - Primary behavior being verified
 * - Edge cases and error conditions handled
 * - Performance requirements met
 * - Real-world applicability confirmed
 * - Professional audio standards maintained
 */
```

### Comment Categories

**🎯 TEST Block**: Describes what specific behavior is being tested
**🎬 SCENARIO Block**: Paints the real-world picture (like a movie scene)  
**✅ VALIDATES Block**: Lists concrete verifications and why they matter
**🎮 SIMULATE Comments**: Describe user/system actions being simulated
**🔍 VERIFY Comments**: Explain what each assertion proves

### Example Transformations

**❌ BEFORE (Basic):**
```cpp
TEST_UNIT(Queue, BasicTest) {
    queue.enqueue(event);
    ASSERT_EQ(1, queue.size());
}
```

**✅ AFTER (Professional):**
```cpp
TEST_UNIT(Queue, BasicPriorityOrdering) {
    /**
     * TEST: Priority queue maintains strict ordering regardless of insertion order
     * 
     * SCENARIO: Live performance where MIDI events arrive out of order:
     *           - SysEx dump starts downloading (LOW priority)
     *           - MIDI clock keeps tempo (CRITICAL priority) 
     *           - User plays notes (HIGH priority)
     * 
     * VALIDATES:
     * - Critical events (clock, start) are processed first
     * - Musical timing accuracy is maintained
     * - System handles mixed priority scenarios professionally
     */
    
    // SIMULATE: Mixed priority events arriving in random order
    queue.enqueue(sysex_event, LOW);
    queue.enqueue(clock_event, CRITICAL);
    
    // VERIFY: Critical event processed first despite later insertion
    ASSERT_EQ(clock_event, queue.dequeue());
}
```

### Key Principles

1. **Tell a Story**: Each test should read like a chapter in the system's story
2. **Real-World Context**: Connect tests to actual use cases (live performance, studio recording, etc.)
3. **Professional Language**: Use industry terminology (RT-safe, audio dropout, MIDI timing, etc.)
4. **Clear Verification**: Explain WHY each assertion matters, not just WHAT it checks
5. **User Perspective**: Frame scenarios from user/musician viewpoint

### Required Elements

- **Scenario description** explaining the real-world situation
- **Validation list** explaining what behaviors are being verified
- **Simulation comments** describing the actions being performed
- **Verification comments** explaining what each assertion proves
- **Context about why** this test matters for audio software

This style makes tests self-documenting and helps new developers understand both the technical implementation and the real-world requirements.
