// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include "SkyboltWidgets/SkyboltWidgetsFwd.h"

#include <QObject>
#include <QVariant>
#include <memory>

namespace skybolt {

struct QtValue : public QObject
{
	Q_OBJECT
public:
	const QVariant& value() const { return mValue; }

	void setValue(const QVariant& v)
	{
		if (mValue != v)
		{
			mValue = v;
			emit valueChanged();
		}
	}


signals:
	void valueChanged();

private:
	QVariant mValue;
};

QtValuePtr createQtValue(const QVariant& value);

struct QtProperty : public QObject
{
	Q_OBJECT
public:
	QString displayName;
	bool enabled = true;

	void setEnabled(bool e)
	{
		if (enabled != e)
		{
			enabled = e;
			emit enabledChanged(enabled);
		}
	}

	//! Never null
	const QtValuePtr& value() const { return mValue; }

signals:
	void enabledChanged(bool enabled);

private:
	QtValuePtr mValue = createQtValue(QVariant{}); //!< never null
};

QtPropertyPtr createQtProperty(const QString& displayName, const QVariant& value);

class PropertiesModel : public QObject
{
	Q_OBJECT
public:
	using Properties = std::vector<QtPropertyPtr>;
	using SectionProperties = std::map<std::string, Properties>;

	PropertiesModel();
	PropertiesModel(const SectionProperties& properties);
	~PropertiesModel() {}

	void update();

	virtual SectionProperties getProperties() const { return mProperties; }

	struct QtValueUpdaterContext {};
	using QtValueUpdater = std::function<void(QtValue&, const QtValueUpdaterContext& context)> ;

	struct QtValueApplierContext {};
	using QtValueApplier = std::function<void(const QtValue&, const QtValueApplierContext& context)>;

	static const std::string& getDefaultSectionName()
	{
		static std::string s;
		return s;
	}

	//! @param updater is regularly called update the value of QtProperty from an external model
	//! @param applier is called when a QtProperty value should be applied to an external model (e.g. if the user pressent 'Enter' key in a text box
	void addProperty(const QtPropertyPtr& property, QtValueUpdater updater = nullptr, QtValueApplier applier = nullptr, const std::string& sectionName = getDefaultSectionName());

signals:
	void modelReset(PropertiesModel*);

protected:
	SectionProperties mProperties;
	std::map<QtPropertyPtr, QtValueUpdater> mPropertyUpdaters;
	bool mCurrentlyUpdating = false;
};

} // namespace skybolt