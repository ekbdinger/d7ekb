<?php

/**
 * @file
 * @brief User Badges admin functions
 *
 * This file contains all the admin functions used by the module.
 *
 * @author Jeff Robbins (jjeff), http://drupal.org/user/17190
 * @author Chad Phillips (hunmonk), http://drupal.org/user/22079
 * @author Heine Deelstra (Heine), http://drupal.org/user/17943
 * @author Nuno Veloso (nunoveloso18), http://drupal.org/user/80656
 * @author Richard Skinner (Likeless), http://drupal.org/user/310635
 * @author Nancy Wichmann (NancyDru), http://drupal.org/user/101412
 * @author Ehud Shahak (basik.drupal) http://drupal.org/user/134140
 */

/**
 * Form builder; list of badges
 */
function user_badges_badgelist_form($form, &$form_state) {
  $show_users = (bool) variable_get('user_badges_list_show_users', 0);
  // Load the badges that we want to display.
  $header = array(
    'name' => array('data' => t('Name'), 'field' => 'name'),
    'badge' => array('data' => t('Badge')),
    'weight' => array('data' => t('Weight'), 'field' => 'weight'),
    'options' => array('data' => t('Options')),
    );

  if ($show_users) {
    $header['users'] = array('data' => t('Assigned to'));
  }

  $header['edit'] = array('data' => t('Edit'));
  $header['delete'] = array('data' => t('Delete'));

  // Which will be the default sort order?
  if (variable_get('user_badges_userweight', 0)) {
    $header['weight']['sort'] = 'asc';
  }
  else {
    $header['name']['sort'] = 'asc';
  }
  
  $form['#header'] = $header;

  $result = db_select('user_badges_badges', 'ubb')
    ->fields('ubb')
    ->extend('TableSort')
    ->orderByHeader($header)
    ->extend('PagerDefault')
    ->limit(50)
    ->execute();

  // Build a table listing the appropriate badges.
  foreach ($result as $badge) {
    $badge->class = 'badge badge-$badge->bid' . drupal_html_class($badge->name);

    $form['name'][$badge->bid] = array('#value' => check_plain($badge->name));

    $form['badge'][$badge->bid] = array(
      '#value' => theme('user_badge', array('badge' => $badge)));

    $form['weight'][$badge->bid] = array(
      '#type' => 'textfield',
      '#size' => 4,
      '#maxlength' => 255,
      '#default_value' => $badge->weight,
    );

    $options = array();
    if ($badge->unhideable) {
      $options[] = t('Cannot be hidden');
    }
    if ($badge->doesnotcounttolimit) {
      $options[] = t('Does not count to limit');
    }

    $form['options'][$badge->bid] = array(
      '#type' => 'value',
      '#value' => implode("<br />", $options),
    );

    if ($show_users) {

      $query = db_select('user_badges_user', 'ubu');
      $query->join('users', 'u', 'u.uid = ubu.uid');
      $query->fields('u', array('uid', 'name'))
        ->condition('ubu.bid', $badge->bid);
      $users = $query->execute();

      $u_list = array();
      foreach ($users as $acct) {
        $u_list[$acct->uid] = l(format_username($acct), "user/$acct->uid/badges");
      }
      if (!$u_list) {
        $u_list[] = t('None');
      }

      $form['users'][$badge->bid] = array('#value' => theme('item_list', array('items' => $u_list)));
    }

    $form['edit'][$badge->bid] = array('#value' => l(t('edit'),  BASE_ADMIN_PATH . '/edit/' . $badge->bid));

    $form['delete'][$badge->bid] = array('#value' => l(t('delete'), BASE_ADMIN_PATH . '/delete/' . $badge->bid));
  }

  $form['submit'] = array(
    '#submit' => t('Update'),
  );

  $form['pager'] = array('#value' => theme('pager', array('tags' => NULL, 'element' => 0)));

  return $form;
}

/**
 * Validate user_badges_badgelist_form form submissions.
 *
 * All weights should be numeric.
 */
function user_badges_badgelist_form_validate($form, &$form_state) {
  if (isset($form['weight']) && is_array($form['weight'])) {
    foreach (element_children($form['weight']) as $bid) {
      if (!is_numeric($form_state['values'][$bid])) {
        form_set_error($bid, t('All weight values must be numeric.'));
      }
    }
  }
}

/**
 * Process user_badges_badgelist_form form submissions.
 *
 * Update the badge weights.
 */
function user_badges_badgelist_form_submit($form, &$form_state) {
  if (isset($form['weight']) && is_array($form['weight'])) {
    foreach (element_children($form['weight']) as $bid) {
      db_update('user_badges_badges')
        ->fields(array('weight' => $form_state['values'][$bid]))
        ->condition('bid', $bid)
        ->execute();
    }
    drupal_set_message(t('The badge weights have been updated.'));
  }
}

/**
 * Theme the badge list form.
 *
 * @param $form
 *   An associative array containing the structure of the form.
 * @ingroup themeable
 */
function theme_user_badges_badgelist_form($variables) {
  $form = $variables['form'];
  $output = '';

  // Loop through the array items in the name array to get all the bids for our listed badges.
  if (isset($form['name']) && is_array($form['name'])) {
    foreach (element_children($form['name']) as $key) {
      //We only want bids as values of $key
      if (!is_numeric($key)) {
        continue;
      }

      // Create the rows array for the table theme.
      $row = array();
      $row[] = $form['name'][$key]['#value'];
      $row[] = $form['badge'][$key]['#value'];
      $row[] = $form['weight'][$key]['#value'];
      $row[] = $form['options'][$key]['#value'];
      if (variable_get('user_badges_list_show_users', 0)) {
        $row[] = $form['users'][$key]['#value'];
      }
      $row[] = $form['edit'][$key]['#value'];
      $row[] = $form['delete'][$key]['#value'];
      $rows[] = $row;
    }

    // Add the submit button.
    $rows[] = array(
      array(
        'data' => drupal_render($form['submit']),
        'align' => 'center',
        'colspan' => '20',
      ),
    );
  }
  else {
    $rows[] = array(array(
        'data' => t('No badges available.'),
        'colspan' => '5',
      ));
  }

  // Theme all that we have processed so far into a table.
  $output .= theme('table', array('header' => $form['#header'], 'rows' => $rows));

  // Create the table's pager.
  if ($form['pager']['#value']) {
    $output .= drupal_render($form['pager']);
  }

  return $output;
}

/**
 * Define the edit form for userbadges.
 */
function user_badges_edit_form($form, $form_state, $bid = NULL) {
  // If we have been given an existing badge (bid) then get all the badge info into $edit.
  if (is_numeric($bid)) {
    $edit = db_select('user_badges_badges', 'ubb')
      ->fields('ubb')
      ->condition('bid', $bid)
      ->execute();

    if ($edit->rowCount() > 0) {
      $form['bid'] = array(
        '#type' => 'value',
        '#value' => $bid,
      );

      $edit = $edit->fetchObject();
    }
  }

  // Are we using a library image or an image URL?
  $libraryimage = '';
  $imageurl = '';
  if (isset($edit) && $edit->image) {
    $libraryimage = $edit->image;
  }

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => isset($edit) ? $edit->name : '',
    '#size' => 40,
    '#maxlength' => 100,
    '#description' => t('Name for the badge. Will be displayed as a tooltip when rolling over the badge.'),
    '#required' => TRUE,
    '#weight' => -10,
    );

  $form['imageurl'] = array(
    '#type' => 'textfield',
    '#title' => t('Image URL'),
    '#default_value' => $imageurl,
    '#size' => 60,
    '#maxlength' => 255,
    '#description' => t('The image URL for this badge. If you want to use an image from the user badges image library, select from the list below.'),
    '#weight' => -8,
    );

  $form['weight'] = array(
    '#type' => 'textfield',
    '#title' => t('Weight'),
    '#size' => 4,
    '#maxlength' => 10,
    '#default_value' => isset($edit) ? $edit->weight : 0,
    '#description' => t('Lighter weighted items float to the top of lists. Heavier items go to the bottom. You must enter a number (negative values are allowed).'),
    '#required' => TRUE,
    '#weight' => -7,
    );

  $form['href'] = array(
    '#type' => 'textfield',
    '#title' => t('Description URL'),
    '#size' => 60,
    '#maxlength' => 255,
    '#description' => t('You can specify here the link where your badge will redirect your user.
      This is useful for explanation pages about the badge, for instance. If you do not wish
      your badge to be clickable, please leave this field empty.') . '<br />' .
      '<u>' . t('Tips:') . '</u>' . '<ul>' .
        '<li>' . t('If you provide the full URL, it will be considered an external URL.') . '</li>' .
        '<li>' . t('If you provide only the path (e.g. "admin/content/node"), it is considered an
          internal link.') . '</li>' .
        '<li>' . t('Use %none to override a default link in the settings tab with no link for this badge.', array('%none' => '<none>')) . '</li>' .
      '</ul>',
    '#default_value' => !empty($edit) ? $edit->href : '',
    '#weight' => -6,
    );

  // Tokens help.
  if (module_exists('token')) {
    $form['token_help'] = array(
      '#title' => t('Replacement patterns'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#weight' => -3,
      );

    $form['token_help']['help'] = array(
      '#theme' => 'token_tree',
      '#token_types' => array('userbadge', 'user'),
      );
  }
  else {
    $form['token_help'] = array(
      '#title' => t('Replacement patterns'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#weight' => -3,
      );
    $form['token_help']['help'] = array(
        // Ignore Coder flag on the link.
        '#value' => t('Install the <a href="!link">Token</a> module if you want this URL to include dynamic elements such as badge ID numbers.',
          array('!link' => url('http://drupal.org/project/token', array('absolute' => TRUE)))
          ),
        );
  }

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display options'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => -2,
    );

  $form['options']['unhideable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Cannot be Hidden'),
    '#default_value' => !empty($edit) ? $edit->unhideable : '',
    '#description' => t('If this is set, the badge cannot be hidden by being moved down in weight. It will always show up.'),
    );

  $form['options']['fixedweight'] = array(
    '#type' => 'checkbox',
    '#title' => t('Fixed Weight'),
    '#default_value' => !empty($edit) ? $edit->fixedweight : '',
    '#description' => t('If this is set, the badge cannot have a user assigned weight, regardless of settings.'),
    );

  $form['options']['doesnotcounttolimit'] = array(
    '#type' => 'checkbox',
    '#title' => t('Does Not Count to Limit'),
    '#default_value' => !empty($edit) ? $edit->doesnotcounttolimit : '',
    '#description' => t('If this is set, the badge does not count towards the limit for number of badges to display per user.'),
    );

  if (module_exists('taxonomy')) {
    $badges_vid = variable_get('user_badges_vid', '');

    if (!empty($badges_vid)) {
      $terms = user_badges_get_term_from_vid($badges_vid);
      $form['tid'] = array(
        '#title' => t('Taxonomy Relation'),
        '#type' => 'select',
        '#options' => !empty($terms) ? $terms : array(t('There are no terms in the selected vocabulery')),
        '#default_value' => !empty($edit) ? $edit->tid : '',
        '#weight' => -1,
        );
    }
    else {
      $form['tid']['help'] = array(
        '#value' => t('Activate the taxonomy module if you want to associate badges with taxonomy terms.'),
       '#weight' => -1,
        );
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save badge'),
    '#weight' => 1,
    );

  $selects = array('' => t('Use the image URL above instead of a library image.')) + user_badges_image_selects();

  // Note: using weights from here allows other modules to insert fields above
  // the image list.

  if (count($selects)) {
    $form['image'] = array(
      '#type' => 'radios',
      '#title' => t('Image Library'),
      '#default_value' => $libraryimage,
      '#options' => $selects,
      '#description' => t('If you have not entered an image URL above, you can select an image from this user badges image library to associate with this badge. You can upload additional images to the library under the <a href="@url">images</a> tab.', array('@url' => url(BASE_ADMIN_PATH . '/images'))),
      '#weight' => 10,
      );
  }
  else {
    $form['noimages'] = array(
      '#type' => 'item',
      '#title' => t('Image'),
      '#value' => t('No badge images uploaded.'),
      '#description' => t('You can enter an image URL directly above, but if you want to instead upload your image to the user badges image library, use <a href="@upload_link">this link</a>. Note that private download is incompatible with this library and should use direct URL entry instead.', array('@upload_link' => url(BASE_ADMIN_PATH . '/images'))),
      '#weight' => 10,
      );
  }

  $form['submit2'] = array(
    '#type' => 'submit',
    '#value' => t('Save badge'),
    '#weight' => 100,
    );

  return $form;
}

/**
 * Validate user_badges_edit_form form submissions.
 */
function user_badges_edit_form_validate($form, &$form_state) {
  // Either a URL or an item from the image library should be selected, but not both.
  if ($form_state['values']['imageurl'] && $form_state['values']['image']) {
    form_set_error('image', t('You cannot both enter an image URL and select an image from the library too. A badge can only have one image.'));
  }
  elseif (!$form_state['values']['imageurl'] && !$form_state['values']['image']) {
    form_set_error('image', t('You need to either enter an image URL or select an image from the library. Your badge needs an image.'));
  }

  // The image URL must be a valid url.
  if ($form_state['values']['imageurl'] && !valid_url($form_state['values']['imageurl'], TRUE)) {
    form_set_error('imageurl', t('This is not a valid image URL. You need to enter a complete image URL, including the "http://" at the start.'));
  }

  // Weights must be numeric.
  if (!is_numeric($form_state['values']['weight'])) {
    form_set_error('weight', t('Your value for the weight must be a number. Negative numbers are allowed, but not decimals.'));
  }

}

/**
 * Process user_badges_edit_form form submissions.
 *
 * Inserts the badge into the DB and sets a success message
 */
function user_badges_edit_form_submit($form, &$form_state) {
  $edit = $form_state['values'];

  // Let other modules know about this.
  module_invoke_all('user_badges_edit_badge', $edit);

  $edit = (object) $edit;

  if (isset($edit->bid)) {
    // If the badge already exists, delete it and re-insert it.
    $result = db_merge('user_badges_badges')
      ->key(array('bid' => $edit->bid));
  }
  else {
    // If the badge does not exist, insert it.
    $result = db_insert('user_badges_badges');
  }

  $image = $edit->imageurl ? trim($edit->imageurl) : $edit->image;

  // Add the rest of the fields.
  $result->fields(array(
      'name' => trim($edit->name),
      'image' => $image,
      'weight' => $edit->weight,
      'href' => (!empty($edit->href) ? trim($edit->href) : NULL),
      'unhideable' => $edit->unhideable,
      'fixedweight' => $edit->fixedweight,
      'doesnotcounttolimit' => $edit->doesnotcounttolimit,
      'tid' => (!empty($edit->tid) ? $edit->tid : NULL),
      ))
    ->execute();

  if ($result) {
    drupal_set_message(t('Badge %badgename saved.', array('%badgename' => $edit->name)));
  }
  else {
    drupal_set_message(t('There was a problem saving the badge information into the database.'));
  }

  $form_state['redirect'] = BASE_ADMIN_PATH;
}

/**
 * Form to upload the badge images or to delete existing ones.
 */
function user_badges_images_form($form, $form_state) {
  $form = array('#skip_duplicate_check' => TRUE);
  $form['new']['upload'] = array(
    '#type' => 'managed_file',
    '#title' => t('Upload image'),
    '#upload_location' => 'public://badges',
    );
/*
  $form['new']['attach'] = array(
    '#type' => 'submit',
    '#value' => t('Upload'),
    );

  $form['#attributes']['enctype'] = 'multipart/form-data';
*/
  $selects = user_badges_image_selects();
  if (count($selects)) {
    $form['images'] = array('#tree' => TRUE);
    foreach ($selects as $imagepath => $imageimg) {
      $form['images'][$imagepath] = array(
        '#type' => 'checkbox',
        '#title' => $imageimg,
        '#return_value' => 1,
        '#default_value' => FALSE,
        '#description' => check_plain($imagepath),
        );
    }
    $form['delete_image'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
  }
  return $form;
}

/**
 * Validate the uploaded image
 *
 * Check whether:
 * Delete has been chosen AND a checkbox has been selected
 * OR
 * Upload has been chosen AND the file upload form is not empty.
 */
function _user_badges_images_form_validate($form, &$form_state) {
  $op = $form_state['clicked_button']['#value'];
  if ($op == t('Upload')) {
    $dir = 'public://badges';

    $is_writable = file_prepare_directory($dir, 1);
    if ($is_writable) {
      $validators = array(
        'file_validate_extensions' => array('png jpg jpeg gif'),
      );

      if ($file = file_save_upload('upload', $validators, $dir)) {
        if (!image_get_info($file->uri)) {
          file_delete($file->uri);
          form_set_error('upload', t('Uploaded image file does not appear to be a valid image file.  Please try again'));
        }
        else {
          user_badges_hold_temporary_file($file);
          $form_state['values']['file_image'] = $file;
        }
      }
      else {
        form_set_error('upload', t('Cannot save image.  Enter the path to an image and try again.'));
      }
    }
    else {
      form_set_error('upload', t('Cannot save image - directory not writable'));
    }
  }
  elseif ($op == t('Delete')) {
    if (count(array_filter($form_state['values']['images'])) == 0) {
      form_set_error('images', t('Please select images to delete.'));
    }
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function user_badges_hold_temporary_file($file = NULL) {
  static $static_file;
  if (isset($file)) {
    $static_file = $file;
  }
  return $file;
}

/**
 * Submission action for user_badges_images_form
 *
 * Save the uploaded file or delete the selected one
 */
function user_badges_images_form_submit($form, &$form_state) {
  $op = $form_state['clicked_button']['#value'];
  switch ($op) {
    case t('Upload'):
      // Save uploaded files.
//      $file = $form_state['values']['file_image'];
      $file = file_load($form_state['values']['file_image']);
      $file->status = FILE_STATUS_PERMANENT;
      $file = file_save($file);
      break;

    case t('Delete'):
      foreach ($form_state['values']['images'] as $path => $is_removed) {
        if ($is_removed) {
          $to_delete[] = $path;
        }
      }
      if (is_array($to_delete)) {
        user_badges_image_delete($to_delete);
      }
      break;
  }
}

/**
 * Delete the specified image.
 */
function user_badges_image_delete($to_delete) {
  $dir = 'public://badges/';

  $is_writable = file_prepare_directory($dir, 1);
  if ($is_writable) {
    foreach ($to_delete as $path) {
      $fid = db_select('file_managed', 'f')
        ->fields('f', array('fid'))
        ->condition('f.uri', $path)
        ->execute()
        ->fetchField();

      $file = file_load($fid);
      file_delete($file);
    }
  }
}

/**
 * Form to associated badges with roles.
 */
function user_badges_roles_form($form, &$form_state) {
  // No badges for the anonymous role.
  $roles = user_roles(TRUE);
  $badges = user_badges_get_roles(NULL, array('returnbadges' => TRUE));

  $form['blocked'] = array(
    '#type' => 'fieldset',
    '#title' => t('Blocked Badge'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );
  $form['blocked'][0] = array(
    '#type' => 'textfield',
    '#title' => t('blocked user'),
    '#size' => 40,
    '#maxlength' => 255,
    '#autocomplete_path' => 'user_badges/autocomplete',
    '#default_value' => isset($badges[0]) ? $badges[0]->name . ' ' . t('(Badge ID') . ' ' . $badges[0]->bid . ')' : '',
    '#field_suffix' => isset($badges[0]) ? ' ' . t('Current:') . ' ' . $badges[0]->image : '',
  );

  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Role Badges'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );

  foreach ($roles as $rid => $role) {
    $form['roles'][$rid] = array(
      '#type' => 'textfield',
      '#title' => check_plain($role),
      '#maxlength' => 255,
      '#autocomplete_path' => 'user_badges/autocomplete',
      '#default_value' => isset($badges[$rid]) ? $badges[$rid]->name . ' '
        . t('(Badge ID') . ' ' . $badges[$rid]->bid . ')' : '',
      '#field_suffix' => isset($badges[$rid]) ?
        ' ' . t('Current:') . ' ' . _user_badges_build_image($badges[$rid]) : '',
    );
  }

  $form[] = array(
    '#type' => 'submit',
    '#value' => t('Save Role Badges'),
  );
  return $form;
}

/**
 * Validation function for user_badges_roles_form.
 */
function user_badges_roles_form_validate($form, &$form_state) {
  $array = $form_state['values']['roles'] + $form_state['values']['blocked'];
  // Go through all the entries and make sure they all have a valid badge ID.
  foreach ($array as $field => $value) {
    if (!empty($value)) {
      // The field isn't empty, so we should validate it.
      $validation = user_badges_badge_autocomplete_validation($value);
      //Is it correctly formatted?
      if ($validation[1] == 'string') {
        if ($field == 0) {
          form_set_error('blocked][' . $field, t('"@value" is not a valid badge name. Try using the autocomplete function (requires javascript).', array('@value' => $value)));
        }
        else {
          form_set_error('roles][' . $field, t('"@value" is not a valid badge name. Try using the autocomplete function (requires javascript).', array('@value' => $value)));
        }
      }
      // The format was correct, but we need to check the bid exists.
      elseif ($validation[1] == 'nobid') {
        if ($field == 0) {
          form_set_error('blocked][' . $field, t('@value is not a valid badge ID. Try using the autocomplete function (requires javascript).', array('@value' => $validation[0])));
        }
        else {
          form_set_error('roles][' . $field, t('@value is not a valid badge ID. Try using the autocomplete function (requires javascript).', array('@value' => $validation[0])));
        }
      }
    }
  }
}

/**
 * Submission function for user_badges_roles_form.
 */
function user_badges_roles_form_submit($form, &$form_state) {
  $array = $form_state['values']['roles'] + $form_state['values']['blocked'];
  foreach ($array as $field => &$value) {
    // We now have a string as our badge, so just extract the bid.
    preg_match('/\(' . t('Badge ID') . ' (\d+)\)/', $value, $matches);
    // Transform our value into just the bid.
    if (!empty($matches)) {
      $value = $matches[1];
    }
  }
  user_badges_save_roles($array);
}

/**
 * Form for general module settings.
 */
function user_badges_settings_form($form, &$form_state) {
  $noyes = array(t('No'), t('Yes'));

  $form['showone'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of badges to display'),
    '#size' => 4,
    '#maxlength' => 4,
    '#default_value' => variable_get('user_badges_showone', 0),
    '#description' => t('Only this many badges with the lightest weights will be shown.')
      . t('Set to zero to apply no limit.')
      . t('Note that if multiple badges have the same lightest weight, only one of them will appear (first by alphabetical order).'),
    );

  $form['showblocked'] = array(
    '#type' => 'radios',
    '#options' => $noyes,
    '#title' => t('Only show blocked user badge'),
    '#default_value' => variable_get('user_badges_showblocked', 0),
    '#description' => t('If checked, only the badge associated to blocked users will be shown, overriding other badges the user eventually has as well as any other settings.') . ' ' .
      t('Note that if there is no badge associated to blocked users, no badges will appear.') . ' ' .
      t('This option only acts on blocked users and has no repercussions on active user badges.'),
    '#attributes' => array('class' => array('container-inline')),
    );

  $form['userweight'] = array(
    '#type' => 'radios',
    '#options' => $noyes,
    '#title' => t('Allow users to reorder badges'),
    '#default_value' => variable_get('user_badges_userweight', 0),
    '#description' => t('If checked, users will have the ability to reweight their badges in their profile, enabling them to set what order their badges display, and also which badges will display if a limit is set above.') . ' ' .
      t('Note that you can make individual badges exempt from this function, allowing you to force them to the top or bottom of the list by giving them a very high or low weight value.'),
    '#attributes' => array('class' => array('container-inline')),
    );

  $form['show_users'] = array(
    '#type' => 'radios',
    '#options' => $noyes,
    '#title' => t('Show users in badge list'),
    '#default_value' => variable_get('user_badges_list_show_users', 0),
    '#description' => t('If selected, the user badges list will include all of the users who have been assigned the badge.'),
    '#attributes' => array('class' => array('container-inline')),
    );

  $form['imagecache'] = array(
    '#type' => 'select',
    '#options' => image_style_options(),
    '#title' => t('Image style to size badges'),
    '#default_value' => variable_get('user_badges_imagecache', ''),
    '#description' => t('This determines the size of the image display.'),
    '#attributes' => array('class' => array('container-inline')),
    );

  $form['nobadges'] = array(
    '#type' => 'textfield',
    '#title' => t('"No badges" message'),
    '#size' => 60,
    '#maxlength' => 500,
    '#default_value' => variable_get('user_badges_nobadges', ''),
    '#description' => t('This provides this message if the user has no badges, rather than nothing. Leave it blank for no message.'),
    );

  $form['defaulthref'] = array(
    '#type' => 'textfield',
    '#title' => t('Default badge link URL'),
    '#size' => 60,
    '#maxlength' => 500,
    '#default_value' => variable_get('user_badges_defaulthref', ''),
    '#description' => t('Enter a default URL to link all badges to. You can override this for individual badges.'),
    );

  // Tokens help.
  if (module_exists('token')) {
    include_once(drupal_get_path('module', 'user_badges') . '/user_badges.tokens.inc');
    $form['token_help'] = array(
      '#title' => t('Replacement patterns'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      );

    $form['token_help']['help'] = array(
      '#theme' => 'token_tree',
      '#token_types' => array('userbadge', 'user'),
      );
  }
  else {
    $form['token_help']['help'] = array(
      // Ignore Coder flag on link.
      '#value' => t('Install the <a href="!link">Token</a> module if you want this URL to include dynamic elements such as badge ID numbers.', array('!link' => url( 'http://drupal.org/project/token', array('absolute' => TRUE)))),
      );
  }

  // Let the user select a vocabulary to associate with badges.
  if (module_exists('taxonomy')) {
    //Build the options for the element
    $vocabs = taxonomy_get_vocabularies();
    $selects = array('' => t('&lt;none&gt;'));
    foreach ($vocabs as $vid => $vocab) {
      $selects[$vid] = $vocab->name;
    }

    $form['vid'] = array(
      '#type' => 'radios',
      '#title' => t('Vocabulary'),
      '#default_value' => variable_get('user_badges_vid', ''),
      '#options' => $selects,
      '#description' => t('Optional. Select a vocabulary to associate badges with. You can optionally associate each badge with one term in this vocabulary.'),
      '#attributes' => array('class' => array('container-inline')),
      );
  }

  $form[] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
    '#weight' => 100,
    );
  return $form;
}

/**
 * Validate user_badges_settings_form input.
 */
function user_badges_settings_form_validate($form, $form_state) {
  // 'showone' must be numeric.
  if (!is_numeric($form_state['values']['showone']) || $form_state['values']['showone'] < 0 ) {
    form_set_error('showone', t('Must be a positive number.'));
  }
}

/**
 * Submission function for user_badges_settings_form.
 *
 * Set the variables and display a message about the set values.
 */
function user_badges_settings_form_submit($form, $form_state) {
  variable_set('user_badges_showone', $form_state['values']['showone']);
  variable_set('user_badges_showblocked', $form_state['values']['showblocked']);
  variable_set('user_badges_userweight', $form_state['values']['userweight']);
  variable_set('user_badges_list_show_users', $form_state['values']['show_users']);
  variable_set('user_badges_imagecache', $form_state['values']['imagecache']);
  variable_set('user_badges_nobadges', $form_state['values']['nobadges']);

  // Only set a default href if we have some non-whitespace in there.
  $defaulthref = trim($form_state['values']['defaulthref']);
  if (empty($defaulthref)) {
    variable_del('user_badges_defaulthref');
  }
  else {
    variable_set('user_badges_defaulthref', $form_state['values']['defaulthref']);
  }

  // Only set a vid if we have a value, otherwise wipe it for none/no taxonomy.
  if ($form_state['values']['vid']) {
    variable_set('user_badges_vid', $form_state['values']['vid']);
  }
  else {
    variable_del('user_badges_vid');
  }

  drupal_set_message(t('Configuration saved.'));
}

/**
 * Select options for displaying a list of images in the image library.
 *
 * @return
 *   An array of images from the image library, in the form $filename => $image_html
 */
function user_badges_image_selects() {
  $selects = array();

  $path = 'public://badges';
  file_prepare_directory($path, FILE_CREATE_DIRECTORY);
  $dir = drupal_realpath($path);

  $files = file_scan_directory($dir, '/.*\.(((J|j)(p|P)(g|G))|((p|P)(n|N)(g|G))|((g|G)(i|I)(f|F)))/', array('recurse' => FALSE));
  foreach ($files as $file) {
    $image = image_get_info($file->uri);
    $file->filepath = $path . '/' . $file->filename;

    $style_name = variable_get('user_badges_imagecache', '');
    $variables = array('path' => $file->filepath);

    if ($style_name) {
      $theme = 'image_style';
      $variables['style_name'] = $style_name;
    }
    else {
      $theme = 'image';
      $variables['width'] = $image['width'];
      $variables['height'] = $image['height'];
    }

    $selects[$file->filepath] = theme($theme, $variables);
  }

  return $selects;
}

/**
 * Get terms belonging to a vocabulary. Can be used in the form's #options field.
 *
 * @param vid
 *   The vocabulary's id.
 * @return
 *   Array whose key is the tid and value the term's name.
 */
function user_badges_get_term_from_vid($vid) {
    $query = db_select('taxonomy_term_data', 't');
    $query->join('taxonomy_term_hierarchy', 'h', 'h.tid = t.tid');
    $query->fields('t', array('tid', 'name'));
    $query->condition('t.vid', $vid, '=');
    $query->condition('h.parent', 0, '='); // remove this line to get all terms.
    $result = $query->execute();

    $data = array();
    while ($record = $result->fetchAssoc()) {
        $data[$record['tid']] = t($record['name']);
    }

    return $data;
}
