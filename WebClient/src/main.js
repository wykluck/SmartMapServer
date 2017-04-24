goog.provide('app');

goog.require('ol.Map');
goog.require('ol.View');
goog.require('ol.layer.Image');
goog.require('ol.proj.Projection');
goog.require('ol.control.MousePosition');
goog.require('ol.source.ImageExport');
goog.require('ol.source.ImageSegment');

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
		params: {'LAYER': 'canberra_2005_uncompressed_pyramids.tif'},
		projection: projection		
	  })
	}),
	new ol.layer.Image({
	  source: new ol.source.ImageSegment({
		url: 'http://localhost:12345/image',
		params: {'LAYER': 'canberra_2005_uncompressed_pyramids.tif'},
		projection: projection		
	  })
	})
];
layers[0].setVisible(true);
layers[1].setVisible(true);

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
	  center: [16501, -30350],
	  resolution: 1,
	  projection: projection
	})
});

/*
 $( "#object-size-ranger" ).slider({
      range: true,
      min: 0,
      max: 5000,
      values: [ 500, 2000 ],
	  create: function( event, ui ) {
        //$( "#amount" ).val( "$" + ui.values[ 0 ] + " - $" + ui.values[ 1 ] );
		layers[0].getSource().updateParams({
			minObjSize: ui.values[0],
			maxObjSize: ui.values[1]
		});
      },
      stop: function( event, ui ) {
        //$( "#amount" ).val( "$" + ui.values[ 0 ] + " - $" + ui.values[ 1 ] );
		layers[0].getSource().updateParams({
			minObjSize: ui.values[0],
			maxObjSize: ui.values[1]
		});
      }
    });
*/
