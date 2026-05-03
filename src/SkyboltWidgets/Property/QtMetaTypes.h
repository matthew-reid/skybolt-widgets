// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include "QtProperty.h"

namespace skybolt {

//! Holds an optional property, where the property may or may not be present (e.g. for std::optional properties)
struct OptionalProperty
{
	QtPropertyPtr property;
	bool present = false;
};

//! Holds a vector of properties of the same type, along with the default value for new items added to the vector
struct PropertyVector
{
	std::vector<QtPropertyPtr> items;
	QVariant itemDefaultValue;
};

//! Holds a tuple of properties of different types.
//! Conceptually, PropertyTuple differs from PropertyVector in that:
//! 1) Properties cannot be added or removed from a PropertyTuple, as opposed to PropertyVector where items can be added or removed.
//! 2) Properties in a PropertyTuple can be of different types, as opposed to PropertyVector where all items must be of the same type.
struct PropertyTuple
{
	std::vector<QtPropertyPtr> items;
};

} // namespace skybolt

Q_DECLARE_METATYPE(skybolt::OptionalProperty)
Q_DECLARE_METATYPE(skybolt::PropertyVector)
Q_DECLARE_METATYPE(skybolt::PropertyTuple)