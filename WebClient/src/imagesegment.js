// FIXME cannot be shared between maps with different projections

goog.provide('ol.source.ImageSegment');

goog.require('ol');
goog.require('ol.Image');
goog.require('ol.asserts');
goog.require('ol.events');
goog.require('ol.events.EventType');
goog.require('ol.extent');
goog.require('ol.obj');
goog.require('ol.source.Image');
goog.require('ol.source.WMSServerType');
goog.require('ol.uri');


/**
 * @classdesc
 * Source for WMS servers providing single, untiled images.
 *
 * @constructor
 * @fires ol.source.Image.Event
 * @extends {ol.source.Image}
 * @param {olx.source.ImageWMSOptions=} opt_options Options.
 * @api
 */
ol.source.ImageSegment = function(opt_options) {

  var options = opt_options || {};

  ol.source.Image.call(this, {
    attributions: options.attributions,
    logo: options.logo,
    projection: options.projection,
    resolutions: options.resolutions
  });

  /**
   * @private
   * @type {?string}
   */
  this.crossOrigin_ =
      options.crossOrigin !== undefined ? options.crossOrigin : null;

  /**
   * @private
   * @type {string|undefined}
   */
  this.url_ = options.url;

  /**
   * @private
   * @type {ol.ImageLoadFunctionType}
   */
  this.imageLoadFunction_ = options.imageLoadFunction !== undefined ?
      options.imageLoadFunction : ol.source.Image.defaultImageLoadFunction;

  /**
   * @private
   * @type {!Object}
   */
  this.params_ = options.params || {};


  /**
   * @private
   * @type {ol.source.WMSServerType|undefined}
   */
  this.serverType_ =
      /** @type {ol.source.WMSServerType|undefined} */ (options.serverType);

  /**
   * @private
   * @type {boolean}
   */
  this.hidpi_ = options.hidpi !== undefined ? options.hidpi : true;

  /**
   * @private
   * @type {ol.Image}
   */
  this.image_ = null;

  /**
   * @private
   * @type {ol.Size}
   */
  this.imageSize_ = [0, 0];

  /**
   * @private
   * @type {number}
   */
  this.renderedRevision_ = 0;

  /**
   * @private
   * @type {number}
   */
  this.ratio_ = options.ratio !== undefined ? options.ratio : 1.5;

};
ol.inherits(ol.source.ImageSegment, ol.source.Image);


/**
 * @const
 * @type {ol.Size}
 * @private
 */
ol.source.ImageSegment.GETFEATUREINFO_IMAGE_SIZE_ = [101, 101];


/**
 * Return the GetFeatureInfo URL for the passed coordinate, resolution, and
 * projection. Return `undefined` if the GetFeatureInfo URL cannot be
 * constructed.
 * @param {ol.Coordinate} coordinate Coordinate.
 * @param {number} resolution Resolution.
 * @param {ol.ProjectionLike} projection Projection.
 * @param {!Object} params GetFeatureInfo params. `INFO_FORMAT` at least should
 *     be provided. If `QUERY_LAYERS` is not provided then the layers specified
 *     in the `LAYERS` parameter will be used. `VERSION` should not be
 *     specified here.
 * @return {string|undefined} GetFeatureInfo URL.
 * @api
 
ol.source.ImageSegment.prototype.getGetFeatureInfoUrl = function(coordinate, resolution, projection, params) {
  if (this.url_ === undefined) {
    return undefined;
  }

  var extent = ol.extent.getForViewAndSize(
      coordinate, resolution, 0,
      ol.source.ImageSegment.GETFEATUREINFO_IMAGE_SIZE_);

  var baseParams = {
    'SERVICE': 'WMS',
    'VERSION': ol.DEFAULT_WMS_VERSION,
    'REQUEST': 'GetFeatureInfo',
    'FORMAT': 'image/png',
    'TRANSPARENT': true,
    'QUERY_LAYERS': this.params_['LAYERS']
  };
  ol.obj.assign(baseParams, this.params_, params);

  var x = Math.floor((coordinate[0] - extent[0]) / resolution);
  var y = Math.floor((extent[3] - coordinate[1]) / resolution);
  baseParams[this.v13_ ? 'I' : 'X'] = x;
  baseParams[this.v13_ ? 'J' : 'Y'] = y;

  return this.getRequestUrl_(
      extent, ol.source.ImageSegment.GETFEATUREINFO_IMAGE_SIZE_,
      1, ol.proj.get(projection), baseParams);
};
*/

/**
 * Get the user-provided params, i.e. those passed to the constructor through
 * the "params" option, and possibly updated using the updateParams method.
 * @return {Object} Params.
 * @api
 */
ol.source.ImageSegment.prototype.getParams = function() {
  return this.params_;
};


/**
 * @inheritDoc
 */
ol.source.ImageSegment.prototype.getImageInternal = function(extent, resolution, pixelRatio, projection) {

  if (this.url_ === undefined) {
    return null;
  }

  resolution = this.findNearestResolution(resolution);

  if (pixelRatio != 1 && (!this.hidpi_ || this.serverType_ === undefined)) {
    pixelRatio = 1;
  }

  var imageResolution = resolution / pixelRatio;

  var center = ol.extent.getCenter(extent);
  var viewWidth = Math.ceil(ol.extent.getWidth(extent) / imageResolution);
  var viewHeight = Math.ceil(ol.extent.getHeight(extent) / imageResolution);
  var viewExtent = ol.extent.getForViewAndSize(center, resolution, 0,
      [viewWidth, viewHeight]);
  var requestWidth = Math.ceil(this.ratio_ * ol.extent.getWidth(extent) / imageResolution);
  var requestHeight = Math.ceil(this.ratio_ * ol.extent.getHeight(extent) / imageResolution);
  var requestExtent = ol.extent.getForViewAndSize(center, resolution, 0,
      [requestWidth, requestHeight]);

  var image = this.image_;
  if (image &&
      this.renderedRevision_ == this.getRevision() &&
      image.getResolution() == resolution &&
      image.getPixelRatio() == pixelRatio &&
      ol.extent.containsExtent(image.getExtent(), viewExtent)) {
    return image;
  }

  var params = {
    'REQUEST': 'segment',
    'FORMAT': 'image/png'
  };
  ol.obj.assign(params, this.params_);

  this.imageSize_[0] = Math.round(ol.extent.getWidth(requestExtent) / imageResolution);
  this.imageSize_[1] = Math.round(ol.extent.getHeight(requestExtent) / imageResolution);

  var url = this.getRequestUrl_(requestExtent, this.imageSize_, params);

  this.image_ = new ol.Image(requestExtent, resolution, pixelRatio,
      this.getAttributions(), url, this.crossOrigin_, this.imageLoadFunction_);

  this.renderedRevision_ = this.getRevision();

  ol.events.listen(this.image_, ol.events.EventType.CHANGE,
      this.handleImageChange, this);

  return this.image_;

};


/**
 * Return the image load function of the source.
 * @return {ol.ImageLoadFunctionType} The image load function.
 * @api
 */
ol.source.ImageSegment.prototype.getImageLoadFunction = function() {
  return this.imageLoadFunction_;
};


/**
 * @param {ol.Extent} extent Extent.
 * @param {ol.Size} size Size.
 * @param {Object} params Params.
 * @return {string} Request URL.
 * @private
 */
ol.source.ImageSegment.prototype.getRequestUrl_ = function(extent, size, params) {

  ol.asserts.assert(this.url_ !== undefined, 9); // `url` must be configured or set using `#setUrl()`
  
  var tempUrl = this.url_.concat('/', params['LAYER'], '/', params['REQUEST']);
  delete params['LAYER'];
  delete params['REQUEST'];
  
  if (!('STYLES' in this.params_)) {
    params['STYLES'] = '';
  }
/*
  if (pixelRatio != 1) {
    switch (this.serverType_) {
      case ol.source.WMSServerType.GEOSERVER:
        var dpi = (90 * pixelRatio + 0.5) | 0;
        if ('FORMAT_OPTIONS' in params) {
          params['FORMAT_OPTIONS'] += ';dpi:' + dpi;
        } else {
          params['FORMAT_OPTIONS'] = 'dpi:' + dpi;
        }
        break;
      case ol.source.WMSServerType.MAPSERVER:
        params['MAP_RESOLUTION'] = 90 * pixelRatio;
        break;
      case ol.source.WMSServerType.CARMENTA_SERVER:
      case ol.source.WMSServerType.QGIS:
        params['DPI'] = 90 * pixelRatio;
        break;
      default:
        ol.asserts.assert(false, 8); // Unknown `serverType` configured
        break;
    }
  }
*/
  params['WIDTH'] = size[0];
  params['HEIGHT'] = size[1];
  //swap y extents and put negative on them so that it is convert to image coordinates
  var tempExtent = extent.slice();
  var tempY = tempExtent[1];
  tempExtent[1] = -tempExtent[3];
  tempExtent[3] = -tempY;
  params['BBOX'] = tempExtent.join(',');
  //TODO: can probably add color and minObjSize and maxObjSize parameters.
  
  return ol.uri.appendParams(/** @type {string} */ (tempUrl), params);
};


/**
 * Return the URL used for this WMS source.
 * @return {string|undefined} URL.
 * @api
 */
ol.source.ImageSegment.prototype.getUrl = function() {
  return this.url_;
};


/**
 * Set the image load function of the source.
 * @param {ol.ImageLoadFunctionType} imageLoadFunction Image load function.
 * @api
 */
ol.source.ImageSegment.prototype.setImageLoadFunction = function(
    imageLoadFunction) {
  this.image_ = null;
  this.imageLoadFunction_ = imageLoadFunction;
  this.changed();
};


/**
 * Set the URL to use for requests.
 * @param {string|undefined} url URL.
 * @api
 */
ol.source.ImageSegment.prototype.setUrl = function(url) {
  if (url != this.url_) {
    this.url_ = url;
    this.image_ = null;
    this.changed();
  }
};


/**
 * Update the user-provided params.
 * @param {Object} params Params.
 * @api
 */
ol.source.ImageSegment.prototype.updateParams = function(params) {
  ol.obj.assign(this.params_, params);
  this.image_ = null;
  this.changed();
};



