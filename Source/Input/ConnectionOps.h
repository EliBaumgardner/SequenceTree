#pragma once

#include "../Util/ApplicationContext.h"
#include "../Util/ArrowInfo.h"

class Arrow;

class ConnectionOps
{
public:

    explicit ConnectionOps(ApplicationContext& context) : applicationContext(context) {}

    void            disconnect        (const Arrow* arrow);
    juce::ValueTree connectionTreeFor (const Arrow* arrow) const;
    void            connect           (int parentNodeId, int childNodeId);

    bool canBeTraversalArrow(const Arrow* arrow) const;
    void setArrowType       (const Arrow* arrow, ArrowType arrowType);

    void applySelectedArrowInfo(int parentNodeId, int childNodeId);

private:

    struct ArrowOwnership
    {
        int ownerNodeId;
        int childNodeId;
    };

    ArrowOwnership resolveOwnership(const Arrow* arrow) const;

    bool connectsToOtherTreeRoot(int parentNodeId, int childNodeId) const;

    ApplicationContext& applicationContext;
};
