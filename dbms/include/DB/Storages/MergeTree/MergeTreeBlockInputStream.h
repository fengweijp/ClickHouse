#pragma once

#include <DB/DataStreams/IProfilingBlockInputStream.h>
#include <DB/Storages/MergeTree/MergeTreeData.h>
#include <DB/Storages/MergeTree/MarkRange.h>


namespace DB
{


class MergeTreeReader;
class UncompressedCache;
class MarkCache;

/// Used to read data from single part.
/// To read data from multiple parts, a Storage creates multiple such objects.
class MergeTreeBlockInputStream : public IProfilingBlockInputStream
{
public:
	MergeTreeBlockInputStream(
		MergeTreeData & storage,
		const MergeTreeData::DataPartPtr & owned_data_part,
		size_t block_size,
		Names column_names,
		const MarkRanges & mark_ranges,
		bool use_uncompressed_cache,
		ExpressionActionsPtr prewhere_actions,
		String prewhere_column,
		bool check_columns,
		size_t min_bytes_to_use_direct_io,
		size_t max_read_buffer_size,
		bool save_marks_in_cache,
		bool quiet = false);

	~MergeTreeBlockInputStream() override;

	String getName() const override { return "MergeTree"; }

	String getID() const override;

protected:
	/// We will call progressImpl manually.
	void progress(const Progress & value) override {}


	/** Если некоторых запрошенных столбцов нет в куске,
	  *  то выясняем, какие столбцы может быть необходимо дополнительно прочитать,
	  *  чтобы можно было вычислить DEFAULT выражение для этих столбцов.
	  * Добавляет их в columns.
	  */
	NameSet injectRequiredColumns(Names & columns) const;

	Block readImpl() override;

private:
	const String path;
	size_t block_size;
	NamesAndTypesList columns;
	NameSet column_name_set;
	NamesAndTypesList pre_columns;
	MergeTreeData & storage;
	MergeTreeData::DataPartPtr owned_data_part;	/// Кусок не будет удалён, пока им владеет этот объект.
	std::unique_ptr<Poco::ScopedReadRWLock> part_columns_lock; /// Не дадим изменить список столбцов куска, пока мы из него читаем.
	MarkRanges all_mark_ranges; /// В каких диапазонах засечек читать. В порядке возрастания номеров.
	MarkRanges remaining_mark_ranges; /// В каких диапазонах засечек еще не прочли.
									  /// В порядке убывания номеров, чтобы можно было выбрасывать из конца.
	bool use_uncompressed_cache;
	std::unique_ptr<MergeTreeReader> reader;
	std::unique_ptr<MergeTreeReader> pre_reader;
	ExpressionActionsPtr prewhere_actions;
	String prewhere_column;
	bool remove_prewhere_column;

	Logger * log;

	/// column names in specific order as expected by other stages
	Names ordered_names;
	bool should_reorder{false};

	size_t min_bytes_to_use_direct_io;
	size_t max_read_buffer_size;

	std::shared_ptr<UncompressedCache> owned_uncompressed_cache;
	std::shared_ptr<MarkCache> owned_mark_cache;
	/// Если выставлено в false - при отсутствии засечек в кэше, считавать засечки, но не сохранять их в кэш, чтобы не вымывать оттуда другие данные.
	bool save_marks_in_cache;
};

}
