/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtilsQt.hpp"
#include "IResourceLibrary.hpp"

#include <functional>

#include <QSet>
#include <QDataStream>
#include <QFile>

namespace FreeHeroes::Core {
inline Q_DECL_PURE_FUNCTION uint qHash(const ResourceMedia::Type & key, uint seed = 0) Q_DECL_NOTHROW {
    return ::qHash(static_cast<int>(key), seed);
}
}

namespace FreeHeroes::Conversion {
class KnownResources;
class ArchiveParser
{
public:
    using std_path = Core::std_path;
    using ExtractCallbackInc = std::function<void()>;
    ArchiveParser(KnownResources& knownResources,
                  QSet<Core::ResourceMedia::Type> requiredTypes,
                  bool overrideExisting,
                  bool keepTmp,
                  ExtractCallbackInc extractCallbackInc
                  );
    enum class TaskType { LOD, HDAT, SND, VID, MusicCopy, DefCopy };
    struct ExtractionTask {
        TaskType type;
        std_path srcRoot;
        std_path srcFilename;
        std_path destResourceRoot;
        Core::IResourceLibrary * resources;
    };
    using ExtractionList = std::vector<ExtractionTask>;

    using ExtractionCallback = std::function<void()>;

    using CallbackInserter = std::function<void(ExtractionCallback)>;

    int estimateExtractCount(const ExtractionList & extractionList);

    void prepareExtractTasks(const ExtractionList & extractionList, CallbackInserter& conversion);


private:
    bool proceed(const ExtractionTask & task, CallbackInserter& conversion, int * estimate);
    bool copyMusic(const ExtractionTask & task, CallbackInserter& conversion, int * estimate);
    bool copyDef(const ExtractionTask & task, CallbackInserter& conversion, int * estimate);
    bool extractLOD(const ExtractionTask & task, CallbackInserter& conversion, int * estimate);
    bool extractHDAT(const ExtractionTask & task, CallbackInserter& conversion, int * estimate);
    bool extractSND(const ExtractionTask & task, CallbackInserter& conversion, int * estimate);
    bool extractVID(const ExtractionTask & task, CallbackInserter& conversion, int * estimate);

    bool needSkipResource(const ExtractionTask & task, const std_path & resourceId, const std_path & resourceExt);
    bool needSkipResource(const ExtractionTask & task, const std_path & resourceId, Core::ResourceMedia::Type type);

private:
    const KnownResources& m_knownResources;
    const QSet<Core::ResourceMedia::Type> m_requiredTypes;

    const bool m_overrideExisting = false;
    const bool m_keepTmp = true;
    ExtractCallbackInc m_extractCallbackInc;
    QFile m_file;
    QDataStream m_ds;
};

}
