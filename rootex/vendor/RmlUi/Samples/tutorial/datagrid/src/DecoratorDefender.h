/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

#ifndef DECORATORDEFENDER_H
#define DECORATORDEFENDER_H

#include <RmlUi/Core/Decorator.h>

/**
	A decorator that displays the defender in the colour of its "colour" property.
	@author Robert Curry
 */

class DecoratorDefender : public Rml::Decorator
{
public:
	virtual ~DecoratorDefender();

	bool Initialise(const Rml::String& image_source, const Rml::String& image_path);

	/// Called on a decorator to generate any required per-element data for a newly decorated element.
	/// @param element[in] The newly decorated element.
	/// @return A handle to a decorator-defined data handle, or nullptr if none is needed for the element.
	Rml::DecoratorDataHandle GenerateElementData(Rml::Element* element) const override;
	/// Called to release element data generated by this decorator.
	/// @param element_data[in] The element data handle to release.
	void ReleaseElementData(Rml::DecoratorDataHandle element_data) const override;

	/// Called to render the decorator on an element.
	/// @param element[in] The element to render the decorator on.
	/// @param element_data[in] The handle to the data generated by the decorator for the element.
	void RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const override;

private:
	int image_index;
};

#endif