define( [
    'dojo/_base/declare',
    'dojo/_base/array',
    'dojo/_base/Deferred',
    'dojo/_base/lang',
    'JBrowse/has',
    'JBrowse/Util',
    'JBrowse/Store/SeqFeature',
    'JBrowse/Store/DeferredStatsMixin',
    'JBrowse/Store/DeferredFeaturesMixin',
    'JBrowse/Model/XHRBlob',
    'JBrowse/Store/SeqFeature/GlobalStatsEstimationMixin',
    './CRAM/File'
],
function(
    declare,
    array,
    Deferred,
    lang,
    has,
    Util,
    SeqFeatureStore,
    DeferredStatsMixin,
    DeferredFeaturesMixin,
    XHRBlob,
    GlobalStatsEstimationMixin,
    CRAMFile
) {
    var CRAMStore = declare( [ SeqFeatureStore, DeferredStatsMixin, DeferredFeaturesMixin, GlobalStatsEstimationMixin ], {
        constructor: function( args ) {
            var cramBlob = args.cram ||
                new XHRBlob(
                    this.resolveUrl(args.urlTemplate || 'data.cram')
                );

            var craiBlob = args.crai ||
                new XHRBlob(
                    this.resolveUrl(args.craiUrlTemplate || ( args.urlTemplate ? args.urlTemplate+'.crai' : 'data.cram.crai' ))
                );

            this.cram = new CRAMFile({
                store: this,
                data: cramBlob,
                crai: craiBlob,
                chunkSizeLimit: args.chunkSizeLimit
            });

            if (!has('typed-arrays')) {
                this._failAllDeferred( 'This web browser lacks support for JavaScript typed arrays.' );
                return;
            }

            this.cram.init({
                success: lang.hitch(this, function() {
                   this._deferred.features.resolve({success:true});

                   this._estimateGlobalStats()
                       .then( lang.hitch(this, function( stats ) {
                              this.globalStats = stats;
                              this._deferred.stats.resolve({success:true});
                          }),
                          lang.hitch( this, '_failAllDeferred' )
                        );
                }),
                failure: lang.hitch( this, '_failAllDeferred' )
            });

            this.storeTimeout = args.storeTimeout || 3000;
        },

        /**
         * Interrogate whether a store has data for a given reference
         * sequence.  Calls the given callback with either true or false.
         *
         * Implemented as a binary interrogation because some stores are
         * smart enough to regularize reference sequence names, while
         * others are not.
         */
        hasRefSeq: function( seqName, callback, errorCallback ) {
            var thisB = this;
            seqName = thisB.browser.regularizeReferenceName( seqName );
            this._deferred.stats.then( function() {
                callback( seqName in thisB.cram.chrToIndex );
            }, errorCallback );
        },

        // called by getFeatures from the DeferredFeaturesMixin
        _getFeatures: function( query, featCallback, endCallback, errorCallback ) {
            this.cram.fetch( this.refSeq.name, query.start, query.end, featCallback, endCallback, errorCallback );
        },

        saveStore: function() {
            return {
                urlTemplate: this.config.cram.url,
                craiUrlTemplate: this.config.crai.url
            };
        }

    });

    return CRAMStore;
});
