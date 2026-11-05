/*
 * Copyright (c) 2026 dresden elektronik ingenieurtechnik gmbh.
 * All rights reserved.
 *
 * The software in this package is published under the terms of the BSD
 * style license a copy of which has been included with this distribution in
 * the LICENSE.txt file.
 *
 */

#include "de_web_plugin.h"
#include "de_web_plugin_private.h"
#include <QSet>
#include "utils/utils.h"

/*! Topology REST API broker.
    \param req - request data
    \param rsp - response data
    \return REQ_READY_SEND
            REQ_NOT_HANDLED
 */
int DeRestPluginPrivate::handleTopologyApi(const ApiRequest &req, ApiResponse &rsp)
{
    // GET /api/<apikey>/topology
    if (req.hdr.pathComponentsCount() == 3 && req.hdr.httpMethod() == HttpGet)
    {
        return getTopology(req, rsp);
    }

    return REQ_NOT_HANDLED;
}

static QString relationName(quint8 relationship)
{
    switch (relationship)
    {
    case 0: return QLatin1String("parent");
    case 1: return QLatin1String("child");
    case 2: return QLatin1String("sibling");
    case 3: return QLatin1String("none");
    case 4: return QLatin1String("previous_child");
    default: return QLatin1String("unknown");
    }
}

/*! GET /api/<apikey>/topology
    Returns the neighbor table links of all nodes.
    Each link is directed: \c source lists \c target as neighbor with the
    given \c lqi as received by \c source.

    Tables received by the plugin in Mgmt_Lqi_rsp come first, with the
    neighbor \c relation, \c depth and \c lastupdated (when the table was
    last completed). Nodes without such a table fall back to the neighbors
    known by the core (source, target and lqi only).
    \return REQ_READY_SEND
            REQ_NOT_HANDLED
 */
int DeRestPluginPrivate::getTopology(const ApiRequest &req, ApiResponse &rsp)
{
    Q_UNUSED(req);

    QVariantList links;
    QSet<quint64> reported;

    for (auto it = neighborTables.cbegin(); it != neighborTables.cend(); ++it)
    {
        if (!it.value().updated.isValid())
        {
            continue;
        }

        reported.insert(it.key());
        const QString source = generateUniqueId(it.key(), 0, 0);
        const QString updated = it.value().updated.toString(QLatin1String("yyyy-MM-ddTHH:mm:ssZ"));

        for (const NeighborTableEntry &entry : it.value().entries)
        {
            if (entry.extAddress == 0 || entry.extAddress == UINT64_MAX)
            {
                continue; // neighbor with unknown extended address
            }

            QVariantMap link;
            link[QLatin1String("source")] = source;
            link[QLatin1String("target")] = generateUniqueId(entry.extAddress, 0, 0);
            link[QLatin1String("lqi")] = int(entry.lqi);
            link[QLatin1String("relation")] = relationName(entry.relationship);
            link[QLatin1String("depth")] = int(entry.depth);
            link[QLatin1String("lastupdated")] = updated;
            links.append(link);
        }
    }

    const deCONZ::Node *node = nullptr;

    for (int i = 0; apsCtrl && apsCtrl->getNode(i, &node) == 0; i++)
    {
        if (reported.contains(node->address().ext()))
        {
            continue;
        }

        for (const deCONZ::NodeNeighbor &neighbor : node->neighbors())
        {
            QVariantMap link;
            link[QLatin1String("source")] = generateUniqueId(node->address().ext(), 0, 0);
            link[QLatin1String("target")] = generateUniqueId(neighbor.address().ext(), 0, 0);
            link[QLatin1String("lqi")] = int(neighbor.lqi());
            links.append(link);
        }
    }

    rsp.map[QLatin1String("links")] = links;
    rsp.httpStatus = HttpStatusOk;
    return REQ_READY_SEND;
}
