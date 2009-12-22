/* 
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_RideMetric_h
#define _GC_RideMetric_h 1

#include <QHash>
#include <QString>
#include <QVector>
#include <QSharedPointer>
#include <assert.h>

#include "RideFile.h"

class Zones;

class RideMetric;
typedef QSharedPointer<RideMetric> RideMetricPtr;

struct RideMetric {
    virtual ~RideMetric() {}

    // The string by which we refer to this RideMetric in the code,
    // configuration files, and caches (like stress.cache).  It should
    // not be translated, and it should never be shown to the user.
    virtual QString symbol() const = 0;

    // A short string suitable for showing to the user in the ride
    // summaries, configuration dialogs, etc.  It should be translated
    // using QObject::tr().
    virtual QString name() const = 0;

    // The units in which this RideMetric is expressed.  It should be
    // translated using QObject::tr().
    virtual QString units(bool metric) const = 0;

    // How many digits after the decimal we should show when displaying the
    // value of a RideMetric.
    virtual int precision() const = 0;

    // The actual value of this ride metric, in the units above.
    virtual double value(bool metric) const = 0;

    // Compute the ride metric from a file.
    virtual void compute(const RideFile *ride, 
                         const Zones *zones, 
                         int zoneRange,
                         const QHash<QString,RideMetric*> &deps) = 0;

    // Fill in the value of the ride metric using the mapping provided.  For
    // example, average speed might be specified by the mapping
    //
    //   { "km" => "18.0", "seconds" => "3600" }
    //
    // As the name suggests, we use this function instead of compute() to
    // override the value of a RideMetric using a value specified elsewhere.
    virtual void override(const QMap<QString,QString> &) {
        assert(false);
    }
    virtual bool canAggregate() const { return false; }
    virtual void aggregateWith(const RideMetric &other) { 
        (void) other; 
        assert(false); 
    }
    virtual RideMetric *clone() const = 0;

    static QHash<QString,RideMetricPtr>
    computeMetrics(const RideFile *ride, const Zones *zones,
                   const QStringList &metrics);
};

class AvgRideMetric : public RideMetric {

    protected:

    int count;
    double total;

    public:

    AvgRideMetric() : count(0), total(0.0) {}
    double value(bool) const {
        if (count == 0) return 0.0;
        return total / count;
    }
    void aggregateWith(const RideMetric &other) { 
        assert(symbol() == other.symbol());
        const AvgRideMetric &as = dynamic_cast<const AvgRideMetric&>(other);
        count += as.count;
        total += as.total;
    }
};

class RideMetricFactory {

    static RideMetricFactory *_instance;
    static QVector<QString> noDeps;

    QVector<QString> metricNames;
    QHash<QString,RideMetric*> metrics;
    QHash<QString,QVector<QString>*> dependencyMap;

    RideMetricFactory() {}
    RideMetricFactory(const RideMetricFactory &other);
    RideMetricFactory &operator=(const RideMetricFactory &other);

    public:

    static RideMetricFactory &instance() {
        if (!_instance)
            _instance = new RideMetricFactory();
        return *_instance;
    }

    int metricCount() const { return metricNames.size(); }

    const QString &metricName(int i) const { return metricNames[i]; }

    RideMetric *newMetric(const QString &symbol) const {
        assert(metrics.contains(symbol));
        return metrics.value(symbol)->clone();
    }

    bool addMetric(const RideMetric &metric,
                   const QVector<QString> *deps = NULL) {
        assert(!metrics.contains(metric.symbol()));
        metrics.insert(metric.symbol(), metric.clone());
        metricNames.append(metric.symbol());
        if (deps) {
            QVector<QString> *copy = new QVector<QString>;
            for (int i = 0; i < deps->size(); ++i) {
                assert(metrics.contains((*deps)[i]));
                copy->append((*deps)[i]);
            }
            dependencyMap.insert(metric.symbol(), copy);
        }
        return true;
    }

    const QVector<QString> &dependencies(const QString &symbol) const {
        assert(metrics.contains(symbol));
        QVector<QString> *result = dependencyMap.value(symbol);
        return result ? *result : noDeps;
    }
};

#endif // _GC_RideMetric_h

