<?php
/**
* Implements views_plugin_style.
*/
class views_plugin_style_lineage_nested extends views_plugin_style_list {
  function options_form(&$form, &$form_state) {
    // @todo: include only lineage fields
    $nesting_options = array('' => '<None>');
    foreach ($this->display->handler->get_handlers('field') as $field => $handler) {
      if ($label = $handler->label()) {
        $nesting_options[$field] = $label;
      }
      else {
        $nesting_options[$field] = $handler->ui_name();
      }
    }

    $options = parent::options_form($form, $form_state);
    if (sizeof($nesting_options) > 1) { // one option is <None>
      $form['nesting'] = array(
        '#type' => 'select',
        '#title' => t('Nesting Field'),
        '#options' => $nesting_options,
        '#default_value' => $this->options['nesting'],
        '#description' => t('Select the Lineage field that will control the nesting.'),  
      );
    }
  }  

  function render() {
    if ($this->uses_row_plugin() && empty($this->row_plugin)) {
      vpr('views_plugin_style_default: Missing row plugin');
      return;
    }

    // First group the rows according to the grouping field, if specified.
    $sets = $this->render_grouping($this->view->result, $this->options['grouping']);

    $this->view->row_index = 0;
    foreach ($sets as $title => $records) {
      if ($this->uses_row_plugin()) {
        $rendered = array();
        foreach ($records as $label => $row) {
          $rendered[] = $this->row_plugin->render($row);
          $this->view->row_index++;
        }
      }
      else {
        $rendered = $records;
      }

      // Now, nest each grouping by the nesting field.
      $nested_set = $this->render_nesting($records, $rendered, $this->options['nesting']);

      // @todo Provide theming.
      $output .= $this->nested_list($nested_set, $title, $this->options['type']);

    }
    unset($this->view->row_index);
    return $output;
  }

  function render_nesting($records,$rendered=false,$nesting_field = '') {
    if (!$rendered) {
      $rendered = $records;
    }
    $nested_sets = array();
    if (isset($this->view->field[$nesting_field])) {
      $field_alias = $this->view->field[$nesting_field]->field_alias;
    }

    if ($nesting_field) {
      foreach ($records as $index => $row) {
        $nesting = '';
        // @todo Avoid using eval().
        $eval = "\$nested_sets"
          . "['".str_replace("\n","']['",$row->$field_alias . $index)."']"
          . " = \$rendered['$index'];";
        eval($eval);
        }
    }
    else {
      // Create a single group with an empty grouping field.
      $nested_sets[''] = $records;
    }

    // Now, clean up the labels for each set (stripping weights).
    // @todo If we replace eval() above with a recursion, refactor this.
    $nested_sets = $this->label_sets($nested_sets);
    return $nested_sets;
  }

  function label_sets($nested_sets) {
    $new_sets = array();
    foreach ($nested_sets as $key => $set) {
      $key = lineage_strip_weight($key);
      $new_sets[$key] = $set;
      if (is_array($set)) {
        $new_sets[$key] = $this->label_sets($set);
      }
    }
    return $new_sets;
  }

  function header($header,$list_depth) {
    // Depth starts at one on the first iteration.  
    // H1 and H2 are reserved for titles, so content headers start at H3.
    $depth = $list_depth + 2; 
    
    // Use a header tag if possible; else use a class indicating header depth.
    if ($depth <= 6) {
      return "<h$depth>$header</h$depth>\n";
    }
    else {
      return "<span class=\"h$depth\">$header</span>\n";
    }
  }

  function nested_list($rows, $header, $type, $list_depth = 0) {
    $list_depth++;

    $output = "";
    if (!empty($header)) {
      $output .=  $this->header($header,$list_depth);
    }
    $output .=  "<$type>\n";
    
    foreach ($rows as $key => $row) {
      $output .=  "<li>\n"; // @todo Add classes.
      // If the next child is an array of rows, recurse.
      if (is_array($row)) {
        // @todo Allow type to vary per list? (And how would we store that?)
        $output .= $this->nested_list($row, $key, $type, $list_depth);
      }
      else {
        $output .=  $row;
      }
      $output .=  "</li>\n";
    }
    
    $output .=  "</$type>\n";
    return $output;
  }
}