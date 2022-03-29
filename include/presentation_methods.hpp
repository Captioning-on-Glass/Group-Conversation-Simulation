#ifndef COG_GROUP_CONVO_CPP_PRESENTATION_METHODS_HPP
#define COG_GROUP_CONVO_CPP_PRESENTATION_METHODS_HPP

#include "AppContext.hpp"

/**
 * Renders registered captions (which remain fixed in space, pinned to the body of the person speaking)
 * using the given app context.
 * @param context
 */
void render_registered_captions(const AppContext *context);

#endif //COG_GROUP_CONVO_CPP_PRESENTATION_METHODS_HPP
