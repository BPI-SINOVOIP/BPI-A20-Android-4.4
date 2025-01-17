/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PageScaleConstraintsSet.h"

namespace WebCore {

static const float defaultMinimumScale = 0.25f;
static const float defaultMaximumScale = 5.0f;

PageScaleConstraintsSet::PageScaleConstraintsSet()
    : m_lastContentsWidth(0)
    , m_needsReset(false)
    , m_constraintsDirty(false)
{
    m_finalConstraints = defaultConstraints();
}

PageScaleConstraints PageScaleConstraintsSet::defaultConstraints() const
{
    return PageScaleConstraints(-1, defaultMinimumScale, defaultMaximumScale);
}

void PageScaleConstraintsSet::updatePageDefinedConstraints(const ViewportArguments& arguments, IntSize viewSize, int layoutFallbackWidth)
{
    m_pageDefinedConstraints = arguments.resolve(viewSize, viewSize, layoutFallbackWidth);

    m_constraintsDirty = true;
}

void PageScaleConstraintsSet::setUserAgentConstraints(const PageScaleConstraints& userAgentConstraints)
{
    m_userAgentConstraints = userAgentConstraints;
    m_constraintsDirty = true;
}

PageScaleConstraints PageScaleConstraintsSet::computeConstraintsStack() const
{
    PageScaleConstraints constraints = defaultConstraints();
    constraints.overrideWith(m_pageDefinedConstraints);
    constraints.overrideWith(m_userAgentConstraints);
    return constraints;
}

void PageScaleConstraintsSet::computeFinalConstraints()
{
    m_finalConstraints = computeConstraintsStack();

    m_constraintsDirty = false;
}

void PageScaleConstraintsSet::adjustFinalConstraintsToContentsSize(IntSize viewSize, IntSize contentsSize, int nonOverlayScrollbarWidth)
{
    m_finalConstraints.fitToContentsWidth(contentsSize.width(), viewSize.width() - nonOverlayScrollbarWidth);
}

void PageScaleConstraintsSet::setNeedsReset(bool needsReset)
{
    m_needsReset = needsReset;
    if (needsReset)
        m_constraintsDirty = true;
}

void PageScaleConstraintsSet::didChangeContentsSize(IntSize contentsSize, float pageScaleFactor)
{
    // If a large fixed-width element expanded the size of the document late in
    // loading and our initial scale is not set (or set to be less than the last
    // minimum scale), reset the page scale factor to the new initial scale.
    if (contentsSize.width() > m_lastContentsWidth
        && pageScaleFactor == finalConstraints().minimumScale
        && computeConstraintsStack().initialScale < finalConstraints().minimumScale)
        setNeedsReset(true);

    m_constraintsDirty = true;
    m_lastContentsWidth = contentsSize.width();
}

static float computeDeprecatedTargetDensityDPIFactor(const ViewportArguments& arguments, float deviceScaleFactor)
{
    if (arguments.deprecatedTargetDensityDPI == ViewportArguments::ValueDeviceDPI)
        return 1.0f / deviceScaleFactor;

    float targetDPI = -1.0f;
    if (arguments.deprecatedTargetDensityDPI == ViewportArguments::ValueLowDPI)
        targetDPI = 120.0f;
    else if (arguments.deprecatedTargetDensityDPI == ViewportArguments::ValueMediumDPI)
        targetDPI = 160.0f;
    else if (arguments.deprecatedTargetDensityDPI == ViewportArguments::ValueHighDPI)
        targetDPI = 240.0f;
    else if (arguments.deprecatedTargetDensityDPI != ViewportArguments::ValueAuto)
        targetDPI = arguments.deprecatedTargetDensityDPI;
    return targetDPI > 0 ? 160.0f / targetDPI : 1.0f;
}

static float getLayoutWidthForNonWideViewport(const FloatSize& deviceSize, float initialScale)
{
    return initialScale == -1 ? deviceSize.width() : deviceSize.width() / initialScale;
}

static float computeHeightByAspectRatio(float width, const FloatSize& deviceSize)
{
    return width * (deviceSize.height() / deviceSize.width());
}

void PageScaleConstraintsSet::adjustForAndroidWebViewQuirks(const ViewportArguments& arguments, IntSize viewSize, int layoutFallbackWidth, float deviceScaleFactor, bool supportTargetDensityDPI, bool wideViewportQuirkEnabled, bool useWideViewport, bool loadWithOverviewMode)
{
    if (!supportTargetDensityDPI && !wideViewportQuirkEnabled && loadWithOverviewMode)
        return;

    const float oldInitialScale = m_pageDefinedConstraints.initialScale;
    if (!loadWithOverviewMode) {
        bool resetInitialScale = false;
        if (arguments.zoom == -1) {
            if (arguments.width == -1)
                resetInitialScale = true;
            if (useWideViewport || arguments.width == ViewportArguments::ValueDeviceWidth)
                resetInitialScale = true;
        }
        if (resetInitialScale)
            m_pageDefinedConstraints.initialScale = 1.0f;
    }

    float adjustedLayoutSizeWidth = m_pageDefinedConstraints.layoutSize.width();
    float adjustedLayoutSizeHeight = m_pageDefinedConstraints.layoutSize.height();
    float targetDensityDPIFactor = 1.0f;

    if (supportTargetDensityDPI) {
        targetDensityDPIFactor = computeDeprecatedTargetDensityDPIFactor(arguments, deviceScaleFactor);
        if (m_pageDefinedConstraints.initialScale != -1)
            m_pageDefinedConstraints.initialScale *= targetDensityDPIFactor;
        m_pageDefinedConstraints.minimumScale *= targetDensityDPIFactor;
        m_pageDefinedConstraints.maximumScale *= targetDensityDPIFactor;
        if (wideViewportQuirkEnabled && (!useWideViewport || arguments.width <= 0)) {
            adjustedLayoutSizeWidth /= targetDensityDPIFactor;
            adjustedLayoutSizeHeight /= targetDensityDPIFactor;
        }

        // In the following cases, a bug in the Classic WebView would mean that the viewport meta tag would take
        // precedence over the app specified setInitialScale value. We keep bugward compatibility with the old
        // WebView for legacy apps (the supportTargetDensityDPI case). New apps will see that setInitialScale()
        // overrides what is specified in the viewport tag.
        if (arguments.width == ViewportArguments::ValueAuto && m_pageDefinedConstraints.initialScale == 1.0f) {
            m_userAgentConstraints.initialScale = -1;
        } else if (arguments.width == ViewportArguments::ValueDeviceWidth || arguments.width == 320) {
            m_userAgentConstraints.initialScale = -1;
        }
    }

    if (wideViewportQuirkEnabled) {
        if (useWideViewport && arguments.width == -1 && arguments.zoom != 1.0f) {
            adjustedLayoutSizeWidth = layoutFallbackWidth;
            adjustedLayoutSizeHeight = computeHeightByAspectRatio(adjustedLayoutSizeWidth, viewSize);
        } else if (!useWideViewport) {
            const float nonWideScale = arguments.zoom < 1 && arguments.width != ViewportArguments::ValueDeviceWidth ? -1 : oldInitialScale;
            adjustedLayoutSizeWidth = getLayoutWidthForNonWideViewport(viewSize, nonWideScale) / targetDensityDPIFactor;
            adjustedLayoutSizeHeight = computeHeightByAspectRatio(adjustedLayoutSizeWidth, viewSize);
            if (arguments.zoom < 1) {
                m_pageDefinedConstraints.initialScale = targetDensityDPIFactor;
                m_pageDefinedConstraints.minimumScale = std::min<float>(m_pageDefinedConstraints.minimumScale, m_pageDefinedConstraints.initialScale);
                m_pageDefinedConstraints.maximumScale = std::max<float>(m_pageDefinedConstraints.maximumScale, m_pageDefinedConstraints.initialScale);
            }
        }
    }

    m_pageDefinedConstraints.layoutSize.setWidth(adjustedLayoutSizeWidth);
    m_pageDefinedConstraints.layoutSize.setHeight(adjustedLayoutSizeHeight);
}

} // namespace WebCore
