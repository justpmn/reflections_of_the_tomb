
#pragma once

class Door
{
public:
    Door();

    // Open the temple door.
    void open();

    // Close the temple door.
    void close();

    // Restore the initial closed state.
    void reset();

    // Check whether the door is open.
    bool isOpen() const;

private:
    bool opened;
};
