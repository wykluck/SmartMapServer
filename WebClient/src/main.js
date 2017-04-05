goog.provide('app');

goog.require('ol.Map');
goog.require('ol.View');
goog.require('ol.layer.Image');
goog.require('ol.proj.Projection');
goog.require('ol.control.MousePosition');
goog.require('ol.source.ImageExport');

var projection = new ol.proj.Projection({
        code: 'CRS:1',
        units: 'pixels',
		axisOrientation: 'edu',
		//getPointResolution: function(viewResolution, point) {
		//	return 1;
		//}
      });
	  
ol.proj.addProjection(projection);


var layers = [
	new ol.layer.Image({
	  source: new ol.source.ImageExport({
		url: 'http://localhost:12345/image',
		params: {'LAYER': '04_072_uncompressed_15759_15756.tif'},
		projection: projection		
	  })
	})
];

var mousePositionControl = 
	new ol.control.MousePosition({
            projection: projection,
            coordinateFormat: ol.coordinate.createStringXY(4),
            className: 'custom-mouse-position',
            target: document.getElementById('mouse-position'),
            undefinedHTML: '&nbsp;'
        });
/**
 * @type {ol.Map}
 */
app.map = new ol.Map({
	controls: ol.control.defaults({
                attributionOptions: /** @type {olx.control.AttributionOptions} */ ({
                    collapsible: false
               })
            }).extend([mousePositionControl])
			,
	layers: layers,
	target: 'map',
	view: new ol.View({
	  center: [7879, -7878],
	  resolution: 1,
	  projection: projection
	})
});

