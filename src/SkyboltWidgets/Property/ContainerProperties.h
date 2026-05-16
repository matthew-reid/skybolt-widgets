// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include "QtProperty.h"

namespace skybolt {

//! Represents an optional value, where the property may or may not be present (e.g. for std::optional properties)
struct QtOptionalValue
{
	QtValuePtr value = createQtValue(QVariant{}); //!< never null;
	bool present = false;
};

//! Represents a vector of properties of the same type, along with the default value for new items added to the vector
struct QtVectorValue
{
	std::vector<QtValuePtr> items;

	using ItemFactory = std::function<QtValuePtr()>;
	ItemFactory itemFactory; //!< Factory function to create new item values, used when adding new items to the vector. Must not be null.
};

//! Represents a struct with of properties
struct QtStructValue
{
	std::vector<QtPropertyPtr> items; //!< Items must not be null
};

QtPropertyPtr createOptionalQtProperty(const QString& displayName, const QVariant& optionalValue, bool present);

QtPropertyPtr createVectorQtProperty(const QString& displayName, const std::vector<QVariant>& values, const QVariant& itemDefaultValue);

} // namespace skybolt

Q_DECLARE_METATYPE(skybolt::QtOptionalValue)
Q_DECLARE_METATYPE(skybolt::QtVectorValue)
Q_DECLARE_METATYPE(skybolt::QtStructValue)