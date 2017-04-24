// Create a simple layer switcher in element div:
var LayerSwitcher = function(options){
  var o = this.options = options || {};
  var map = this.map = options.map;
  var cssPath = o.cssPath || 'css/LayerSwitcher.css';
  
  // element to render in:
  var $div;
  if(!o.div){
    $div = $('<div class="LayerSwitcher">'); $(document.body).append($div);
    o.div = $div[0];
  } else {
    var $div = typeof o.div == 'string' ? $('#'+o.div) : $(o.div);
    $div.addClass("LayerSwitcher");
  }
  this.div = o.div;
  
  var $baseDiv = $('<div class="BaseLayerDiv">');
  var $overDiv = $('<div class="OverlayDiv">');
  $div.append($baseDiv, $overDiv);
  
  // load css:
  //var cssL = document.createElement('link'); 
  var $cssL = $('<link>'), cssL = $cssL[0];
  cssL.rel = 'stylesheet'; cssL.type = 'text/css'; cssL.href = cssPath;
  $(document.head).append(cssL);
  
  // array with layers:
  var layers = map.getLayers().getArray();
  
  // turn off other baselayers:
  var otherBLoff = function(layer){
    $.each(layers, function(i,l){
      if(l!==layer && l.get('baselayer'))
        { l.setVisible(false); }
    });
  };
  
  // go through each layer, render control and set handlers:
  $.each(layers, function(i,l){
    var BL = l.get('baselayer');
    var $li = $('<div class="check">');
      l.getVisible() ? $li.addClass('checked') : $li.removeClass('checked') ;
      BL ? $li.addClass('radiobutton') : $li.addClass('checkbox') ;
    var $ll = $('<label>'+l.get('name')+'</label>');
    var $ld = $('<div class="LayerClickDiv">').click(function(){ 
      l.setVisible(!l.getVisible());
      l.get('baselayer') ? otherBLoff(l) :0;
    }); //toggle viz on click
    $ld.append($li,$ll);
    BL ? $baseDiv.append($ld) : $overDiv.append($ld) ;
    // bind checkbox state to layer event:
    l.on('change:visible', function(e){
      this.getVisible() ? $li.addClass('checked') : $li.removeClass('checked') ;
    }); // bind
  }); // each
  if($baseDiv.children()[0] && $overDiv.children()[0]){
    $baseDiv.after('<div class="Separator">');
  }
  
}; // LayerSwitcher