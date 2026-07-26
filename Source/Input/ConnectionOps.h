#pragma once

#include "../Util/ApplicationContext.h"

class Arrow;

class ConnectionOps
{
public:

    explicit ConnectionOps(ApplicationContext& context) : applicationContext(context) {}

    void            disconnect        (const Arrow* arrow);
    juce::ValueTree connectionTreeFor (const Arrow* arrow) const;
    void            connect           (int parentNodeId, int childNodeId);

private:

    struct ArrowOwnership
    {
        int ownerNodeId;
        int childNodeId;
    };

    ArrowOwnership resolveOwnership(const Arrow* arrow) const;

    ApplicationContext& applicationContext;
};
