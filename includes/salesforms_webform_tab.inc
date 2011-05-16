<?php
// $Id$

/**
 * @file
 *   The field map form for the main Salesforms tab on the Webform page
 */

/**
 * The form to assign Salesforce maps to Drupal form fields
 * for the Salesforms webform tab
 */

function salesforms_config_form(&$form_state, $node) {
  $form = array();
  
  $link = l('help page', 'admin/help/salesforms');
  
  $form['description'] = array(
    '#value' => t('<p>For more information, read the !help</p>', array('!help' => $link)),
  );
  
  // Grab the current field map for the webform
  // @TODO
  //   Is there a cleaner way to get the form_id?
  $map = salesforms_get_map('webform_client_form_' . $node->nid);
  
  // Grab the dropdown code from the generation function
  $salesforms_dropdown = _salesforms_generate_dropdown();
  
  // Step through each webform component, and create a corresponding dropdown field.
  // If a map already exists, set the mapped field as the default value.
  foreach ($node->webform['components'] as $cid => $component) {
    $key = $component['form_key'];
    $form['submitted['.$key.']'] = $salesforms_dropdown;
    $form['submitted['.$key.']']['#title'] = $component['name'] . ' - ' . $component['type'];
    
    if (isset($map[$key])) {
      $form['submitted['.$key.']']['#default_value'] = $map[$key]['fid'];
      
      if(!empty($map[$key]['reference_to'])) {
        $form['submitted['.$key.'_external_id]'] = array(
          '#type' => 'textfield',
          '#title' => $map[$key]['field_name'] . ' External Id',
          '#description' => t('If this field refers to an existing record, enter an external id'),
        );
        
        // Default value is empty in SQL table
        if (!empty($map[$key]['external_id'])) {
          $form['submitted['.$key.'_external_id]']['#default_value'] = $map[$key]['external_id'];
        }
      }
    }
  }
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save',
  );
  
  return $form;
}


/**
 * Submission handler for the Salesforms webform tab
 *
 * @return drupal_set_message()
 *   Confirmation message
 *
 * @TODO
 *   Prevent assigning of multiple form fields to the same SF field
 */

function salesforms_config_form_submit($form, &$form_state) {
  // Build the form id of the webform from the nid
  $node = node_load(arg(1));
  $webform_id = 'webform_client_form_' . arg(1);
  $map = salesforms_get_map($webform_id);
  
  // Get rid of all field maps to make sure that old records aren't hanging around from deleted fields
  db_query( "DELETE FROM {salesforms_fieldmaps} WHERE form_id = '%s'", $webform_id );
  
  // Collect the posted data
  $post = $form['#post']['submitted'];
  // Step through the posted data, and assign field maps where appropriate
  // @TODO Make this a single query as in https://drupal.org/node/1021018#comment-3925462
  
  foreach ($node->webform['components'] as $cid => $component) {
    $key = $component['form_key'];
    
    // If there is a field id set
    if ($post[$key] !== 'none') {
      if (isset($post[$key.'_external_id']) && !empty($post[$key.'_external_id'])) {
        $external_id = $post[$key.'_external_id'];
        $internal_id = salesforms_get_by_external($map[$key]['reference_to'], $external_id);
      }
      else {
        $external_id = $internal_id = '';
      }
      
      db_query("INSERT INTO {salesforms_fieldmaps} (fid, form_id, form_field, external_id, internal_id)
                VALUES (%d, '%s', '%s', '%s', '%s')",
                $post[$key], $webform_id, $key, $external_id, $internal_id
              );
    }
  }
  
  return drupal_set_message(t('Salesforms field maps updated'));
}


/**
 * Returns a table of salesforce objects for each form submission
 */

function salesforms_object_results($node) {
  $header = array(
    'Submission Id',
    'Saved Objects'
  );
  
  $rows = array();
  
  $query = db_query("SELECT t1.sid, object_type, object_id
                     FROM {".SALESFORMS_OBJECT_TABLE."} AS t1
                     JOIN {webform_submissions} AS t2
                     ON t1.sid = t2.sid
                     WHERE t2.nid = %d",
                     $node->nid
                   );
  
  while ($result = db_fetch_array($query)) {
    $rows[$result['sid']] = array(
      $result['sid'],
    );
    
    $objects[$result['sid']][] = l($result['object_type'], 'http://na7.salesforce.com/' . $result['object_id']);
  }
  
  foreach ($rows as $sid => $row) {
    $rows[$sid][] = implode(', ', $objects[$sid]);
  };
  
  return theme_table($header, $rows);
}