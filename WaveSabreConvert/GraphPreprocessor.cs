using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using static WaveSabreConvert.Song;

namespace WaveSabreConvert
{
    public struct SongExecutionPlan
    {
        // maximum number of threads per batch.
        public int MaxThreads;

        public struct BatchEntry
        {
            public int OriginalTrackIndex;
            public int ExecOrderTrackIndex; // track index in execution order
            public int BatchIndex; // index of the batch this track belongs to
            public bool IsLastInBatch;
        }

        public List<BatchEntry> ExecutionOrder; // list of all tracks in execution order, with batch info.

        public BatchEntry GetBatchEntryForExecutionOrder(int execOrderTrackIndex)
        {
            return ExecutionOrder[execOrderTrackIndex];
        }

        public BatchEntry GetBatchEntryForTrack(int oldTrackIndex)
        {
            foreach (var entry in ExecutionOrder)
            {
                if (entry.OriginalTrackIndex == oldTrackIndex)
                {
                    return entry;
                }
            }
            throw new InvalidDataException($"Track index {oldTrackIndex} not found in execution plan.");
        }

        public int RemapOriginalTrackIndexToExecutionOrder(int oldTrackIndex)
        {
            return GetBatchEntryForTrack(oldTrackIndex).ExecOrderTrackIndex;
        }
    }

    public class GraphPreprocessor
    {
        private static HashSet<int> GetDependenciesForTrack(Song song, int trackIndex)
        {
            var trackCount = song.Tracks.Count;
            var dependencies = new HashSet<int>();
            foreach (var receive in song.Tracks[trackIndex].Receives)
            {
                var dependencyTrackIndex = receive.SendingTrackIndex;

                if (dependencyTrackIndex < 0 || dependencyTrackIndex >= trackCount)
                {
                    throw new InvalidDataException($"Track {trackIndex} depends on invalid track index {dependencyTrackIndex}.");
                }

                if (dependencyTrackIndex == trackIndex)
                {
                    throw new InvalidDataException($"Track {trackIndex} depends on itself.");
                }

                dependencies.Add(dependencyTrackIndex);
            }
            return dependencies;
        }

        // Leaves the song untouched, analyzes the track processing DAG and
        // returns an execution plan that allows the graph runner to be as tiny as possible.
        // the graph runner can just push tasks and join when needed.
        //
        // first, make sure a thread pool of size MaxThreads is available.
        //
        // for each node
        // {
        //    assign this node to worker
        //    if node.isLastInBatch
        //    {
        //       join workers
        //    }
        // }
        // It means the serializer only needs to output:
        // - max threads (for creating the thread pool)
        // - for each track, a flag signalling if the track is the last one in the batch
        //
        // the song's track list is going to get re-ordered, so the serializer needs to be aware of this
        // and fix-up references to tracks in the song (devices? send/receives? midi lanes?)
        public static SongExecutionPlan CalculateExecutionPlan(Song song, int maxThreads = -1)
        {
            // the algo to do this can take many forms to produce a balanced graph, even estimating
            // the processing time of each node, collapsing chains (bus -> drums -> drumset -> track) into a single node
            // to avoid unnecessary graph depth.
            //
            // but we can start with something simple; something like:

            // while (allNodes.hasItems)
            // {
            //    thisBatch = allNodes[
            //         where all dependencies have been processed
            //         limit max thread count
            //       ]
            //    batches.push(thisBatch);
            // }

            if (song == null) throw new ArgumentNullException(nameof(song));
            if (song.Tracks.Count < 1) throw new ArgumentException("Song must contain at least one track.", nameof(song));

            if (maxThreads <= 0)
            {
                maxThreads = song.Tracks.Count; // no limit, all tracks can be processed in parallel if there are no dependencies.
            }

            SongExecutionPlan plan = new SongExecutionPlan();
            plan.ExecutionOrder = new List<SongExecutionPlan.BatchEntry>(song.Tracks.Count);

            var trackCount = song.Tracks.Count;
            var remainingDependencyCounts = new int[trackCount]; // track index -> number of dependencies that haven't been processed yet.
            var dependents = new List<int>[trackCount]; // track index -> list of tracks that depend on this track.
            for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
            {
                dependents[trackIndex] = new List<int>();
            }

            // build the dependency graph and count the number of dependencies for each track.
            for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
            {
                var dependencies = GetDependenciesForTrack(song, trackIndex);
                foreach (var dependencyTrackIndex in dependencies)
                {
                    dependents[dependencyTrackIndex].Add(trackIndex);
                }
                remainingDependencyCounts[trackIndex] = dependencies.Count;
            }

            // find tracks with no dependencies to start with.
            var readyTracks = new SortedSet<int>();
            for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
            {
                if (remainingDependencyCounts[trackIndex] == 0)
                {
                    readyTracks.Add(trackIndex);
                }
            }

            var processedTrackCount = 0;
            var maxBatchSize = 0;
            var batchIndex = 0;
            while (processedTrackCount < trackCount)
            {
                if (readyTracks.Count == 0)
                {
                    throw new InvalidDataException("Track dependency graph contains a cycle.");
                }

                var batchTrackIndices = new List<int>(Math.Min(maxThreads, readyTracks.Count));

                while (batchTrackIndices.Count < maxThreads && readyTracks.Count > 0)
                {
                    var nextTrackIndex = readyTracks.Min;
                    readyTracks.Remove(nextTrackIndex);
                    batchTrackIndices.Add(nextTrackIndex);
                }

                processedTrackCount += batchTrackIndices.Count;
                maxBatchSize = Math.Max(maxBatchSize, batchTrackIndices.Count);

                for (int batchTrackIndex = 0; batchTrackIndex < batchTrackIndices.Count; batchTrackIndex++)
                {
                    plan.ExecutionOrder.Add(new SongExecutionPlan.BatchEntry
                    {
                        OriginalTrackIndex = batchTrackIndices[batchTrackIndex],
                        ExecOrderTrackIndex = plan.ExecutionOrder.Count,
                        BatchIndex = batchIndex,
                        IsLastInBatch = batchTrackIndex == batchTrackIndices.Count - 1,
                    });
                }

                foreach (var trackIndex in batchTrackIndices)
                {
                    foreach (var dependentTrackIndex in dependents[trackIndex])
                    {
                        remainingDependencyCounts[dependentTrackIndex]--;
                        if (remainingDependencyCounts[dependentTrackIndex] == 0)
                        {
                            readyTracks.Add(dependentTrackIndex);
                        }
                    }
                }

                batchIndex++;
            }

            plan.MaxThreads = maxBatchSize;

            return plan;
        }
    } // class GraphPreprocessor
}
